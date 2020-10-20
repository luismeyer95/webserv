#include <thread>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <vector>
#include <netinet/in.h> 
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <cstring>

constexpr size_t max_buf = 1024 * 128;

int			port;
std::string expect;
ssize_t		expect_size = -1;
size_t		workers = 1;
size_t		request_count = 1;
bool 		print = false;
bool		chunked = false;

void runtime_error(const std::string& err)
{
	std::cout << "error: " << err << std::endl;
	exit(1);
}

void usage_error()
{
	std::cout << 	"usage:\t./bin -p <port_number>\n"
					"\t[-w <number_of_workers>] [-r <requests_per_worker>]\n"
					"\t[--expect-content <response_file>] [--expect-size <response_size>]\n"
					"\t[--chunked] [--print-response]\n";
	exit(1);
}

int open_file(const std::string& filename)
{
	int fd = open(filename.c_str(), O_RDONLY, 0644);
	if (fd == -1)
		runtime_error("could not open file");
	return fd;
}

std::string	dechunk(std::string& chunked_body)
{
	auto process_chunk = [&] (std::string& out) -> size_t {
		size_t hex_break = chunked_body.find("\r\n");
		if (hex_break == std::string::npos)
			return -1;
		std::string hexnumstr = chunked_body.substr(0, hex_break);
		size_t hexnum = 0;
		hexnum = std::stoull(hexnumstr, nullptr, 16); 
		if (chunked_body.size() >= hexnumstr.size() + 2 + hexnum + 2
			&& chunked_body.substr(hex_break + 2 + hexnum).find("\r\n") == 0)
		{
			out.append(chunked_body.substr(hex_break + 2, hexnum));
			chunked_body = chunked_body.substr(hex_break + 2 + hexnum + 2);
			return hexnum;
		}
		else
			return -1;
	};
	ssize_t ret;
	std::string out;
	while ((ret = process_chunk(out)) && ret != -1);
	if (ret == -1)
		runtime_error("chunk processing error");
	return out;
}

std::string fdtostr(int fd)
{
	char buf[max_buf + 1];
	ssize_t ret = 0;
	std::string out;
	
	while ( (ret = read(fd, buf, max_buf)) > 0 )
	{
		buf[ret] = 0;
		out += buf;
	}
	if (ret == -1)
		runtime_error("read(): " + std::string(strerror(errno)));
	return out;
}

void	send_all(int sock, const std::string& bytes)
{
	ssize_t		ret = 0;
	size_t		to_send = bytes.size();
	const char* ptr_bytes = &bytes[0];

	while ((ret = send(sock, ptr_bytes, to_send, 0)) > 0)
	{
		ptr_bytes += ret;
		to_send -= ret;
	}
	if (ret == -1)
		runtime_error("send(): " + std::string(strerror(errno)));
}

void	response_check(std::string& response)
{
	if (print)
	{
		std::cout << "----------------------------------" << std::endl;
		std::cout << response << std::endl;
		std::cout << "----------------------------------" << std::endl;
	}
	if (expect.empty() && expect_size == -1)
		return;

	auto hbreak = response.find("\r\n\r\n");
	if (hbreak == std::string::npos)
		runtime_error("header break not found in response");

	std::string body = response.substr(hbreak + 4);
	if (chunked)
	{
		auto dechunked = dechunk(body);
		body = std::move(dechunked);
	}

	if (expect_size != -1 && body.size() != static_cast<size_t>(expect_size))
		runtime_error("expected size not matched... (" + std::to_string(expect_size) + ")");
	if (!expect.empty() && body != expect)
	{
		std::cout << "received " << body.size() << " vs expected " << expect.size() << std::endl;
		std::cout << "strcmp(received, expected) -> " << body.compare(expect) << std::endl;
		size_t i = 0;
		while (body[i] && expect[i] == body[i])
			++i;
		std::cout << "unmatched index: " << i << std::endl;
		std::cout << "received[i] = " << body[i] << ", expected[i] = " << expect[i] << std::endl;
		runtime_error("expected file not matched...");
	}
}

void	single_request(std::string request)
{
	int sock = 0;
    struct sockaddr_in serv_addr; 
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation error \n"); 
		exit(1);
    }
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(port);
       
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)  
    { 
        printf("\nInvalid address/ Address not supported \n"); 
        exit(1);
    } 
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
        printf("\nConnection Failed \n"); 
        exit(1);
    }

	send_all(sock, request);
	std::string response = fdtostr(sock);
	response_check(response);
	close(sock);
}

void	worker(std::string request)
{
	for (size_t i = 0; i < request_count; ++i)
		single_request(request);
}

void	ddos(std::string request)
{
	std::vector<std::thread> th_list;
	for (size_t i = 0; i < workers; ++i)
	{
		std::thread th(worker, request);
		th_list.push_back(std::move(th));
	}
	for (auto& thrd : th_list)
		thrd.join();
}

std::string	find_setting(const std::vector<std::string>& args, const std::string& setting)
{
	auto f = std::find(args.begin(), args.end(), setting);
	if (f != args.end() && std::next(f) != args.end())
		return *std::next(f);
	return {};
}

void	set_conf_advanced(const std::vector<std::string>& args)
{
	if (std::find(args.begin(), args.end(), "--chunked") != args.end())
		chunked = true;
	if (std::find(args.begin(), args.end(), "--print-response") != args.end())
		print = true;

	auto expect_filename = find_setting(args, "--expect-content");
	if (!expect_filename.empty())
	{
		int fd = open_file(expect_filename);
		expect = fdtostr(fd);
		close(fd);
	}

	auto expect_size_str = find_setting(args, "--expect-size");
	if (!expect_size_str.empty())
		expect_size = std::stoull(expect_size_str);
}


void	set_conf(const std::vector<std::string>& args)
{
	auto port_str = find_setting(args, "-p");
	if (port_str.empty())
		usage_error();
	port = std::stoi(port_str);

	auto workers_str = find_setting(args, "-w");
	if (!workers_str.empty())
	{
		workers = std::stoull(workers_str);
		std::cout << "worker count = " << workers << std::endl;
	}
	else
		std::cout << "defaulting to " << workers << " workers..." << std::endl;

	auto requests_str = find_setting(args, "-r");
	if (!requests_str.empty())
	{
		request_count = std::stoull(requests_str);
		std::cout << "requests/worker = " << request_count << std::endl;
	}
	else
		std::cout << "defaulting to " << request_count << " request/worker..." << std::endl;
	
	set_conf_advanced(args);
	std::cout << std::endl;
}

void	http_print(const std::string& s)
{
	for (size_t i = 0; i < s.size(); ++i)
	{
		if (s[i] == '\n')
			std::cout << "\\n";
		else if (s[i] == '\r')
			std::cout << "\\r";
		else
			std::cout << s[i];
	}
	std::cout << std::endl;
}

void httpify(std::string& input_request)
{
	auto hbreak = input_request.find("\n\n");
	if (hbreak == std::string::npos)
		runtime_error("couldn't find header break in input request");
	
	for (size_t i = 0; i < hbreak + 2; ++i)
		if (input_request.at(i) == '\n')
		{
			input_request.insert(i++, 1, '\r');
			hbreak++;
		}	
}

int main(int ac, char **av)
{
	std::vector<std::string> args(av, av + ac);

	set_conf(args);

	std::cout << "getting input...\n";
	std::cout << "----------------------------------" << std::endl;
	std::string input = fdtostr(0);
	std::cout << "\n----------------------------------" << std::endl;

	httpify(input);
	ddos(input);

	std::cout << "\nall good :)\n";
}
