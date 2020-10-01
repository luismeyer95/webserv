#pragma once

#include <header.h>
#include <RequestBuffer.hpp>

struct HTTPExchange;

struct Socket
{
	int socket_fd;
	virtual bool isListener() = 0;
	virtual ~Socket() {}
};

struct ClientSocket;

struct Listener : Socket
{
	unsigned short				port;
	std::string					address_str;
	struct sockaddr_in			address;
	std::vector<ClientSocket*>	comm_sockets;

	bool						isListener();
};

struct ClientSocket : Socket
{
	Listener*					lstn_socket;
	std::string					client_address;

	// The request buffer stores every incoming byte, when requests are extracted
	// bytes are removed from the buffer until the next request
	RequestBuffer				req_buffer;
	std::queue<HTTPExchange>	exchanges;

	ClientSocket();

	// Takes a finished request from the request buffer, creates a http exchange ticket
	// and advances the buffer to the character after the first crlf (next request) 
	HTTPExchange&				newExchange(const ByteBuffer& buffer);
	HTTPExchange&				getExchange();
	void						closeExchange();
	bool						isListener();

};