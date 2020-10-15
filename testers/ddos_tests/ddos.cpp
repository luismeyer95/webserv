#include <thread>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <vector>
#include <netinet/in.h> 
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

void	single_request(std::string request, int port)
{
	int sock = 0, valread;
    struct sockaddr_in serv_addr; 
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation error \n"); 
		exit(1);
    } 
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(port);
       
    // Convert IPv4 and IPv6 addresses from text to binary form 
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

	char buffer[1025];
    send(sock, request.c_str(), request.size(), 0);
    valread = read(sock, buffer, 1024);
	if (valread != -1)
		buffer[valread] = 0;
	
	close(sock);
}

void	worker(std::string request, int port)
{
	for (int i = 0; i < 50; ++i)
		single_request(request, port);
}

void	ddos(int port, std::string request)
{
	std::vector<std::thread> th_list;
	for (int i = 0; i < 256; ++i)
	{
		std::thread th(worker, request, port);
		th_list.push_back(std::move(th));
	}
	for (auto& thrd : th_list)
		thrd.join();
}

std::string read_stdin()
{
	std::string out;
	std::string line;

	while (std::getline(std::cin, line))
		out += line + "\n";

	return out;
}

int main(int ac, char **av)
{
	if (ac != 2)
	{
		std::cout << "usage: " << av[0] << " " << "<port>\n";
		exit(1);
	}

	ddos(std::stoi(av[1]), read_stdin());
}