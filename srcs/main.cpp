#include "../includes/Logger.hpp"
#include "../includes/ServerSocketPool.hpp"
#include "../includes/Parser.hpp"
#include "../includes/Utils.hpp"
#include "../includes/header.h"
#include "../includes/ByteBuffer.hpp"

// sig_atomic_t& sigflag()
// {
// 	static sig_atomic_t sigflag = 0;
// 	return sigflag;
// }

// void	handle_signal(int sig)
// {
// 	(void)sig;
// 	sigflag() = 1;
// }

void	handle_connection(HTTPExchange& comm)
{
	(void)comm;
	// comm.bufferResponse("Welcome to the server!\r\n\r\n", true);
}

void	handle_request(HTTPExchange& comm)
{
	std::string msg(comm.request);

	std::string get = tokenizer(msg, ' ')[1];

	if (get == "/")
		get = "./simple_site/index.html";
	else
		get = "./simple_site" + get;

	if (get.find("/..") != std::string::npos)
		return;

	ByteBuffer doc;
	std::ostringstream stream;
	stream << "HTTP/1.1 200 OK\r\n";
	stream << "Content-Length: " << doc.peekSize(get) << "\r\n\r\n";
	doc.appendBuffer(stream);
	doc.appendBinaryData(get);

	comm.bufferResponse(doc, true);
}

int main(int ac, char **av)
{
	(void)ac;

	if (ac != 2)
	{
		std::cout << "usage: " << av[0] << " <port>" << std::endl;
		return (0);
	}

	// signal(SIGINT, handle_signal);

	ServerSocketPool& pool = ServerSocketPool::getInstance();
	Logger& log = Logger::getInstance();
	pool.addListener(atoi(av[1]));

	pool.runServer(handle_connection, handle_request);
}