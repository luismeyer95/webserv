#include <Logger.hpp>
#include <ServerSocketPool.hpp>
#include <RequestParser.hpp>
#include <Utils.hpp>
#include <header.h>
#include <ByteBuffer.hpp>

#include <Regex.hpp>
#include <Conf.hpp>

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
	if (ac != 2)
	{
		std::cout << "usage: " << av[0] << " <configuration file>" << std::endl;
		return (0);
	}

	ServerSocketPool& pool = ServerSocketPool::getInstance();
	Logger& log = Logger::getInstance();
	std::vector<std::string> args(av, av + ac);

	log.out() << "Initializing server..." << std::endl;
	log.out() << "Loading configuration file" << std::endl;

	try {
		ConfParser conf(args[1]);
		log.ok();
	} catch (const std::runtime_error& e) {
		log.out() << "[" BOLDRED "ERROR" RESET "]: " << e.what() << std::endl;
		return (0);
	}

	log.out() << "Setting up virtual hosts" << std::endl;
	
	try {
		pool.addListener("localhost", 80);
	} catch (const std::runtime_error& e) {
		return (1);
	}
	pool.runServer(handle_connection, handle_request);
}