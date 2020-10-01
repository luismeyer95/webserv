#pragma once

#include "header.h"
#include "Logger.hpp"
#include "RequestParser.hpp"
#include "ByteBuffer.hpp"
#include "Conf/Config.hpp"
#include "RequestRouter.hpp"
#include <RequestBuffer.hpp>
#include <Sockets.hpp>
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
			const ByteBuffer& req, const std::string& client_address,
			const std::string& address, unsigned short port
		);

		const ByteBuffer	request;
		void				bufferResponse(const ByteBuffer& str, bool mark_end = false);
		ByteBuffer			getResponse();

		std::string			clientAddress();
		std::string			listeningAddress();
		unsigned short		listeningPort();
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

		// std::runtime_error constructorExcept(const std::string& err)
		// {
		// 	for (auto& s : socket_list)
		// 	{
		// 		delete s;
		// 		s = nullptr;
		// 	}
		// 	throw std::runtime_error(err);
		// }

	public:
		typedef ft::deque<Socket*>::iterator iterator;
		ServerSocketPool();
		~ServerSocketPool();

		void				setConfig(RequestRouter conf_handler);

		void				addListener(const std::string& host, unsigned short port);
		ClientSocket*		acceptConnection(Listener* lstn);

		void				initFdset();

		bool				selected(Socket* socket, fd_set* set);
		void				closeComm(ClientSocket* comm);
		ft::deque<Socket*>&	getSocketList();

		size_t				recvRequest(ClientSocket* cli, int& retflags);
		size_t				sendResponse(ClientSocket* cli, int& retflags);

		void				runServer(
			void (*connection_handler)(HTTPExchange&, RequestRouter&) ,
			void (*request_handler)(HTTPExchange&, RequestRouter&)
		);
		void				pollRead(Socket* s);
		void				pollWrite(Socket* s);
};
