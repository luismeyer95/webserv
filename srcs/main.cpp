#include "../includes/Logger.hpp"
#include "../includes/ServerSocketPool.hpp"
#include "../includes/Parser.hpp"
#include "../includes/header.h"


void	handle_connection(HTTPExchange& comm)
{
	comm.bufferResponse("Welcome to the server!\r\n\r\n", true);
}

void	handle_request(HTTPExchange& comm)
{
	std::string msg(comm.request);

	// comm.bufferResponse("HEY HOMIE WASSUP IN THE HOOD MOFO\r\n\r\n", true);
	for (size_t i = 0; i < msg.size(); ++i)
		if (i % 2)
			msg[i] = std::toupper(msg[i]);
	comm.bufferResponse(msg, true);
}

int main(int ac, char **av)
{
	(void)ac;

	ServerSocketPool& pool = ServerSocketPool::getInstance();
	Logger& log = Logger::getInstance();
	pool.addListener(atoi(av[1]));

	pool.runServer(handle_connection, handle_request);
}