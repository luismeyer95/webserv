#include "../includes/Logger.hpp"
#include "../includes/ServerSocketPool.hpp"
#include "../includes/Parser.hpp"
#include "../includes/Utils.hpp"
#include "../includes/header.h"
#include "../includes/ByteBuffer.hpp"

void	handle_connection(HTTPExchange& comm)
{
	(void)comm;
	// comm.bufferResponse("Welcome to the server!\r\n\r\n", true);
}

void	handle_request(HTTPExchange& comm)
{
	std::string msg(comm.request);

	std::string get = tokenizer(msg, ' ')[1];

	std::cout << "TOKEN: :" << get << std::endl;
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
	doc.appendBuffer((BYTE*)stream.str().c_str(), stream.str().size());
	doc.appendBinaryData(get);

	// comm.bufferResponse("HEY HOMIE WASSUP IN THE HOOD MOFO\r\n\r\n", true);
	// for (size_t i = 0; i < msg.size(); ++i)
	// 	if (i % 2)
	// 		msg[i] = std::toupper(msg[i]);
	// comm.bufferResponse(msg, true);

	
	// resp.write((char*)&doc[0], doc.size());

	comm.bufferResponse(doc, true);
}

int main(int ac, char **av)
{
	(void)ac;

	ServerSocketPool& pool = ServerSocketPool::getInstance();
	Logger& log = Logger::getInstance();
	pool.addListener(atoi(av[1]));

	pool.runServer(handle_connection, handle_request);
}