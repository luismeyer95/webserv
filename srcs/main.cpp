#include "../includes/Logger.hpp"
#include "../includes/ServerSocketPool.hpp"
#include "../includes/RequestParser.hpp"
#include "../includes/Utils.hpp"
#include "../includes/header.h"
#include "../includes/ByteBuffer.hpp"

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