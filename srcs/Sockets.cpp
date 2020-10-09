#include <Sockets.hpp>
#include <ServerSocketPool.hpp>

bool 				Listener::isListener()
{
	return true;
}

ClientSocket::ClientSocket(RequestRouter& route)
	: Socket(), lstn_socket(nullptr), req_buffer(route, this)
{

}

bool 				ClientSocket::isListener()
{
	return false;
}

HTTPExchange&		ClientSocket::newExchange(const ByteBuffer& buffer)
{
	HTTPExchange xch (
		buffer, client_address,
		lstn_socket->address_str, lstn_socket->port
	);
	exchanges.push(std::move(xch));
	return exchanges.back();
}

HTTPExchange&		ClientSocket::getExchange()
{
	return exchanges.front();
}

void				ClientSocket::closeExchange()
{
	exchanges.pop();
}
