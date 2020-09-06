#include <Logger.hpp>
#include <ServerSocketPool.hpp>
#include <RequestParser.hpp>
#include <Utils.hpp>
#include <header.h>
#include <ByteBuffer.hpp>

#include <Regex.hpp>

void	handle_connection(HTTPExchange& comm)
{
	(void)comm;
}

void	handle_request(HTTPExchange& comm)
{
	Logger& log = Logger::getInstance();
	// Extracting the resource's path
	std::string msg(comm.request);
	std::string resource = tokenizer(msg, ' ')[1];

	//call RequestParser here

	//GET
	if (resource.find("/..") != std::string::npos)
		return;
	
	if (resource == "/")
		resource = "./simple_site/index.html";
	else
		resource = "./simple_site" + resource;

	// Buffering a generic response for all calls (simply sends 200 OK + the resource's content)
	ByteBuffer doc;
	doc << "HTTP/1.1 200 OK\r\n";
	doc << "Content-Length: " << ByteBuffer::peekFileSize(resource) << "\r\n\r\n";
	doc.appendFile(resource);

	// Load the response in the http exchange ticket and mark as ready
	comm.bufferResponse(doc, true);
}

int main(int ac, char **av)
{

	std::vector<std::string> args(av, av + ac);

	if (ac != 2)
	{
		std::cout << "usage: " << args[0] << " <port>" << std::endl;
		return (0);
	}

	ServerSocketPool& pool = ServerSocketPool::getInstance();
	Logger& log = Logger::getInstance();
	
	pool.addListener(std::stoi(args[1]));
	pool.runServer(handle_connection, handle_request);
}

/* MAIN POUR TESTER LES REGEXS */

// int main(int ac, char **av)
// {
// 	if (ac != 3)
// 	{
// 		std::cout << "usage: " << av[0] << " <pattern> <string_to_match>" << std::endl;
// 		return(1);
// 	}

// 	try {
// 		Regex rgx(av[1]);
// 		std::pair<bool, std::string> p = rgx.match(av[2]);
// 		std::cout << "Match: " << (p.first ? "yes" : "no") << std::endl;
// 		if (p.first)
// 			std::cout << "String matched: \"" << p.second << "\"" << std::endl;
// 	} 
// 	catch (const std::runtime_error& e) {
// 		std::cout << e.what() << std::endl;
// 	}

// 	return 0;
// }