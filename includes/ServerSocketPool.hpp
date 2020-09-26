#pragma once

#include "header.h"
#include "Logger.hpp"
#include "RequestParser.hpp"
#include "ByteBuffer.hpp"
#include "Conf/Config.hpp"
#include "RequestRouter.hpp"
#include <map>
#include <list>
#include <vector>
#include <queue>
#include <deque>

#define MAXBUF 65536
#define MAXQUEUE 30

struct ClientSocket;

struct HTTPExchange
{
	private:
		friend struct	ClientSocket;
		friend class	ServerSocketPool;

		ByteBuffer		response_buffer;
		ByteBuffer		response;
		bool			end;

		std::string		client_address;
		std::string		server_address;
		unsigned short	port;

	public:
		HTTPExchange (
			const std::string& req, const std::string& client_address,
			const std::string& address, unsigned short port
		)
		: response_buffer(), response(), end(false), client_address(client_address),
		  server_address(address), port(port), request(req) {}

		const std::string	request;
		void	bufferResponse(const ByteBuffer& str, bool mark_end = false)
		{
			response_buffer += str;
			response += str;
			end = mark_end;
		}
		ByteBuffer getResponse() { return response; }

		std::string listeningAddress() { return server_address; }
		std::string clientAddress() { return client_address; }
		unsigned short listeningPort() { return port; }
};

struct Socket {
	int socket_fd;
	virtual bool isListener() = 0;
	virtual ~Socket() {}
};

struct Listener : Socket
{
	unsigned short				port;
	std::string					address_str;
	struct sockaddr_in			address;
	std::vector<ClientSocket*>	comm_sockets;
	bool isListener() { return true; }
};

struct ClientSocket : Socket
{
	Listener*					lstn_socket;
	std::string					client_address;
	// The request buffer stores every incoming byte, when requests are extracted
	// bytes are removed from the buffer until the next request
	std::string					req_buffer;
	std::queue<HTTPExchange>	exchanges;

	bool		isListener() { return false; }

	// Takes a finished request from the request buffer, creates a http exchange ticket
	// and advances the buffer to the character after the first crlf (next request) 
	HTTPExchange&		newExchange()
	{
		// TO UPDATE LATER:
		// cutting out the request payload because we don't handle those yet.
		// final build will have the cutoff be at crlf + 4 + len(payload)
		size_t find_end = req_buffer.find("\r\n\r\n");
		HTTPExchange xch (
			req_buffer.substr(0, find_end + 4), client_address,
			lstn_socket->address_str, lstn_socket->port
		);
		exchanges.push(std::move(xch));
		if (!req_buffer.empty())
			req_buffer = req_buffer.substr(find_end + 4);
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

		RequestRouter	conf;

		int		fd_max;
		fd_set	master_read;
		fd_set	master_write;
		ft::deque<Socket*> socket_list;

		void (*connection_handler)(HTTPExchange&, RequestRouter&);
		void (*request_handler)(HTTPExchange&, RequestRouter&);

		std::runtime_error constructorExcept(const std::string& err)
		{
			for (auto& s : socket_list)
			{
				delete s;
				s = nullptr;
			}
			throw std::runtime_error(err);
		}

	public:
		typedef ft::deque<Socket*>::iterator iterator;
		ServerSocketPool();
		~ServerSocketPool();

		void	setConfig(RequestRouter conf_handler);

		void	addListener(const std::string& host, unsigned short port);
		ClientSocket*	acceptConnection(Listener* lstn);
		// bool	portIsUnused(unsigned short port);

		void	initFdset();

		bool	selected(Socket* socket, fd_set* set);
		void	closeComm(ClientSocket* comm);
		ft::deque<Socket*>&	getSocketList();

		size_t	recvRequest(ClientSocket* cli, int& retflags);
		size_t	sendResponse(ClientSocket* cli, int& retflags);

		void	runServer(
			void (*connection_handler)(HTTPExchange&, RequestRouter&) ,
			void (*request_handler)(HTTPExchange&, RequestRouter&)
		);
		void	pollRead(Socket* s);
		void	pollWrite(Socket* s);

		// static ServerSocketPool&	getInstance();
};
