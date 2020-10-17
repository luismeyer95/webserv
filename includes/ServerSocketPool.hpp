#pragma once

#include "header.h"
#include "Logger.hpp"
#include "RequestParser.hpp"
#include "ByteBuffer.hpp"
#include "Conf/Config.hpp"
#include "RequestRouter.hpp"
#include <RequestBuffer.hpp>
#include <RequestParser.hpp>
#include <ResponseBuffer.hpp>
#include <Sockets.hpp>
#include <SharedPtr.hpp>
#include <map>
#include <list>
#include <vector>
#include <queue>
#include <deque>

#define MAXBUF 262144
#define MAXQUEUE 30

struct ClientSocket;

struct HTTPExchange
{
	private:
		friend struct	ClientSocket;
		friend class	ServerSocketPool;
		
		ByteBuffer					response_headers;
		SharedPtr<ResponseBuffer>	response_buffer;
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
		void				bufferResponse(const ByteBuffer& headers, SharedPtr<ResponseBuffer> buf, bool mark_end = false);
		ResponseBuffer&		getResponse();

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

		RequestRouter		conf;

		int					fd_max;
		fd_set				master_read;
		fd_set				master_write;
		std::vector<Socket*>	socket_list;

	public:
		typedef std::vector<Socket*>::iterator iterator;
		ServerSocketPool();
		~ServerSocketPool();

		void				setConfig(RequestRouter conf_handler);

		void				addListener(const std::string& host, unsigned short port);
		ClientSocket*		acceptConnection(Listener* lstn);

		void				initFdset();

		bool				selected(Socket* socket, fd_set* set);
		void				closeComm(ClientSocket* comm);
		std::vector<Socket*>&	getSocketList();

		size_t				recvRequest(ClientSocket* cli, int& retflags);
		size_t				sendResponse(ClientSocket* cli, int& retflags);

		void				runServer();

		void				pollRead(Socket* s);
		bool				pollWrite(Socket* s);
};
