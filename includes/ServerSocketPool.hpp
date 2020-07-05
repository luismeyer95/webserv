#pragma once

#include "header.h"
#include "Logger.hpp"
#include "Parser.hpp"
#include "ByteBuffer.hpp"
#include <map>
#include <list>
#include <vector>
#include <queue>
#include <deque>

#define MAXBUF 16384
#define MAXQUEUE 30

void	http_print(const std::string& s);
void	dec_print(const char *s);

struct ClientSocket;

struct HTTPExchange
{
	private:
		friend struct ClientSocket;
		friend class ServerSocketPool;
		// std::string 		response_buffer;
		// std::string			response;
		ByteBuffer			response_buffer;
		ByteBuffer			response;
		bool				end;
	public:
		HTTPExchange(const std::string& req)
			: response_buffer(), response(), end(false), request(req) {}
			// : response_buffer(""), response(""), end(false), request(req) {}

		const std::string	request;
		void	bufferResponse(const ByteBuffer& str, bool mark_end = false)
		{
			response_buffer += str;
			response += str;
			end = mark_end;
		}
		ByteBuffer getResponse() { return response; }
};

struct Socket {
	int socket_fd;
	virtual bool isListener() = 0;
	virtual ~Socket() {}
};

struct Listener : Socket
{
	unsigned short port;
	struct sockaddr_in 	address;
	std::vector<ClientSocket*>	comm_sockets;
	bool isListener() { return true; }
};

struct ClientSocket : Socket
{
	Listener*					lstn_socket;
	std::string					req_buffer;
	std::queue<HTTPExchange>	exchanges;

	bool		isListener() { return false; }

	HTTPExchange&		newExchange()
	{
		size_t find_end = req_buffer.find("\r\n\r\n");
		exchanges.push(HTTPExchange(req_buffer.substr(0, find_end + 4)));
		// http_print(req_buffer);
		// dec_print(req_buffer.c_str());
		// std::cout << "find end:" << find_end << std::endl;
		// std::cout << "before: " << req_buffer << std::endl;
		// dec_print(req_buffer.c_str());
		if (!req_buffer.empty())
			req_buffer = req_buffer.substr(find_end + 4);
		// std::cout << "after: " << req_buffer << std::endl;
		// dec_print(req_buffer.c_str());
		return exchanges.back();
	}

	HTTPExchange&		getExchange()
	{
		return exchanges.front();
	}

	void				closeExchange()
	{
		exchanges.pop();
	}

	ClientSocket()
		: Socket(), lstn_socket(nullptr) {}
};


class ServerSocketPool
{
	private:

		enum class IOSTATE {
			ONCE = 1,
			READY = 2
		};

		int		fd_max;

		fd_set	master_read;
		fd_set	master_write;

		void (*connection_handler)(HTTPExchange&);
		void (*request_handler)(HTTPExchange&);

		ft::deque<Socket*> socket_list;
		ServerSocketPool();
	public:
		typedef ft::deque<Socket*>::iterator iterator;
		~ServerSocketPool() {}

		void	addListener(unsigned short port);
		ClientSocket*	acceptConnection(Listener* lstn);
		bool	portIsUnused(unsigned short port);

		void	initFdset();

		bool	selected(Socket* socket, fd_set* set);
		void	closeComm(ClientSocket* comm);
		ft::deque<Socket*>&	getSocketList();

		size_t	recvRequest(ClientSocket* cli, int& retflags);
		size_t	sendResponse(ClientSocket* cli, int& retflags);

		void	runServer(
			void (*connection_handler)(HTTPExchange&) ,
			void (*request_handler)(HTTPExchange&)
		);
		void	pollRead(Socket* s);
		void	pollWrite(Socket* s);

		static ServerSocketPool&	getInstance();
};
