#include <ServerSocketPool.hpp>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>


/*		-------------------
**		class HTTPExchange;
**		-------------------
*/

HTTPExchange::HTTPExchange (
	const ByteBuffer& req,
	const std::string& client_address,
	const std::string& address, unsigned short port
) : response_buffer(), end(false),
	client_address(client_address),server_address(address),
	port(port), request(req)
{

}

void	HTTPExchange::bufferResponse(const ByteBuffer& headers, SharedPtr<ResponseBuffer> buf, bool mark_end)
{
	response_headers = headers;
	response_buffer = buf;
	end = mark_end;
}

ResponseBuffer&		HTTPExchange::getResponse()
{
	return *response_buffer;
}

std::string		HTTPExchange::listeningAddress()
{
	return server_address;
}

std::string		HTTPExchange::clientAddress()
{
	return client_address;
}

unsigned short	HTTPExchange::listeningPort()
{
	return port;
}


/*		-------------------
**		class ServerSocketPool;
**		-------------------
*/

ServerSocketPool::ServerSocketPool()
	: fd_max(-1)
{
	
}

void	ServerSocketPool::setConfig(RequestRouter conf)
{
	this->conf = conf;

	ConfBlockDirective& main = *conf.main;

	typedef std::pair<std::string, unsigned short> hp_pair;
	std::set<hp_pair> added_hostports;

	for (auto& b : main.blocks)
	{
		if (b.key == ContextKey::server)
		{
			std::string host_port = RequestRouter::getDirective(b, DirectiveKey::listen).values.at(0);
			auto tokens = tokenizer(host_port, ':');
			std::string host = tokens.at(0);
			if (host == "localhost")
				host = "127.0.0.1";
			unsigned short port = std::stoi(tokens.at(1));

			if (!added_hostports.count(hp_pair(host,port)))
			{
				addListener(host, port);
				added_hostports.insert(hp_pair(host,port));
			}
		}
	}
}

void	ServerSocketPool::addListener(const std::string& host, unsigned short port)
{
	Logger& log = Logger::getInstance();

	uint32_t addrbin = inet_addr(host.c_str());
	log.out() << "Verifying IP `" << host << "`" << std::endl;
	if (addrbin == INADDR_NONE)
		throw std::runtime_error("invalid IP address `" + host + "`");

	int sock = 0;
	log.out() << "Creating virtual host for `" << host << ":" << port << "`\n";
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		throw std::runtime_error("Failed to create socket. " + std::string(strerror(errno)));
	

	Listener* lstn = new Listener();
	lstn->socket_fd = sock;
	lstn->address_str = host;
	lstn->port = port;
	socket_list.push_back(static_cast<Socket*>(lstn));

	lstn->address.sin_family = AF_INET;
	lstn->address.sin_port = htons(port);
	lstn->address.sin_addr.s_addr = addrbin;
	
	int ret = bind(lstn->socket_fd, (struct sockaddr*)&lstn->address, sizeof(lstn->address));
	if (ret == -1)
		throw std::runtime_error (
			"Failed to bind socket to `" + host + ":"
			+ std::to_string(port) + "`. " + strerror(errno) + "."
		);
	log.out() << "Virtual host bound successfully to `" << host << ":" << port << "`\n";

	listen(lstn->socket_fd, MAXQUEUE);
	
	if (lstn->socket_fd > fd_max)
		fd_max = lstn->socket_fd;
}

ServerSocketPool::~ServerSocketPool()
{
	for (iterator it = socket_list.begin(); it != socket_list.end(); ++it)
	{
		if (*it)
		{
			close((*it)->socket_fd);
			FD_CLR((*it)->socket_fd, &master_read);
			FD_CLR((*it)->socket_fd, &master_write);
			delete *it;
			*it = nullptr;
		}
	}
}

ft::deque<Socket*>&		ServerSocketPool::getSocketList()
{
	return socket_list;
}

void handle_sigint(int sig) 
{
	(void)sig;
	write(1, "\n", 1);
}

void	ServerSocketPool::runServer(
	void (*connection_handler)(HTTPExchange&, RequestRouter& conf),
	void (*request_handler)(HTTPExchange&, RequestRouter& conf)
)
{
	this->connection_handler = connection_handler;
	this->request_handler = request_handler;
	initFdset();

	signal(SIGINT, handle_sigint);

	while (true)
	{
		fd_set copy_read = master_read;
		fd_set copy_write = master_write;
		int socket_count = select(fd_max + 1, &copy_read, &copy_write, nullptr, nullptr);
		if (socket_count == -1 && errno == EINTR)
			break;
		
		size_t i = 0;
		size_t size = socket_list.size();
		iterator it = socket_list.begin();
		// Push and pops will happen on the socket list in the poll calls.
		// We need to ensure we don't process those that were created within
		// the select scan loop, because otherwise we will select-test fds
		// that did not go through a select() call, hence why we limit the
		// loop's maximum index to the initial size of the socket list
		while (i < size && it != socket_list.end())
		{
			iterator next = it + 1;
			if (selected(*it, &copy_write))
				pollWrite(*it);
			if (selected(*it, &copy_read))
				pollRead(*it);
			it = next;
			++i;
		}
	}
}

void	ServerSocketPool::initFdset()
{
	FD_ZERO(&master_read);
	FD_ZERO(&master_write);
	for (iterator it = socket_list.begin(); it != socket_list.end(); ++it)
		FD_SET((*it)->socket_fd, &master_read);
}

bool	ServerSocketPool::selected(Socket* socket, fd_set* set)
{
	return FD_ISSET(socket->socket_fd, set);
}


ClientSocket*	ServerSocketPool::acceptConnection(Listener* lstn)
{
	ClientSocket* comm = new ClientSocket();
	comm->lstn_socket = lstn;

	Logger& log = Logger::getInstance(); 
	char ipstr[INET_ADDRSTRLEN];

	struct sockaddr store;
	socklen_t		len = sizeof(store);

	comm->socket_fd = accept(lstn->socket_fd, &store, &len);
	struct sockaddr_in *s = (struct sockaddr_in*)&store;
	inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof(ipstr));
	comm->client_address = ipstr;

	// log.out() << "connection ip = " << ipstr << std::endl;

	fcntl(comm->socket_fd, F_SETFL, O_NONBLOCK);
	lstn->comm_sockets.push_back(comm);
	FD_SET(comm->socket_fd, &master_read);
	socket_list.push_back((Socket*)comm);
	if (comm->socket_fd > fd_max)
		fd_max = comm->socket_fd;
	return comm;
}


void	ServerSocketPool::closeComm(ClientSocket* comm)
{
	std::vector<ClientSocket*>& commlist = comm->lstn_socket->comm_sockets;
	for (std::vector<ClientSocket*>::iterator it = commlist.begin(); it != commlist.end(); ++it)
	{
		if (*it == comm)
		{
			commlist.erase(it);
			break;
		}
	}
	for (iterator it = socket_list.begin(); it != socket_list.end(); ++it)
	{
		if (!(*it)->isListener() && *it == comm)
		{
			socket_list.erase(it);
			close(comm->socket_fd);
			FD_CLR(comm->socket_fd, &master_read);
			FD_CLR(comm->socket_fd, &master_write);
			delete comm;
			comm = nullptr;
			return;
		}
	}
}

void	ServerSocketPool::pollRead(Socket* s)
{
	Logger& log = Logger::getInstance();

	if (s->isListener())
	{
		log.out() << "[connection]\n";
		// create the communication socket for that connection
		ClientSocket* client_socket = acceptConnection((Listener*)s);
	}
	else 
	{
		ClientSocket* cli = static_cast<ClientSocket*>(s);
		int retflags = 0;
		size_t readbytes = recvRequest(cli, retflags);
		if (!(retflags & (int)IOSTATE::ONCE))
		{
			// if socket has been read-selected but recv() read 0 bytes, socket should be closed
			// and removed from the pool
			log.out() << "<disconnect fd=" << cli->socket_fd  << ">" << std::endl;
			closeComm(cli);
		}
		else
		{
			log.out() << "[inbound]: "
				<< "fd="  << cli->socket_fd << ", "
				<< "size=" << readbytes << std::endl;
			if (retflags & (int)IOSTATE::READY)
			{
				// at least one request is fully buffered:
				// - create exchange + call callback
				// - put client fd on write queue
				// - an exchange is done and popped from the queue when
				//	 the request_handler has marked the end of the response
				// - write poller will pop client from the write queue
				//	 once the http exchange pool for this client is empty
				RequestBuffer& buff = cli->req_buffer;
				ByteBuffer msg(buff.extract(false));
				if (msg.strfind("\r\n\r\n") != -1)
					msg = msg.sub(0, msg.strfind("\r\n\r\n"));
				log.out() << "[request]: fd=" << cli->socket_fd << std::endl;
				log.out(msg.str());
				while (cli->req_buffer.ready())
					request_handler(cli->newExchange(buff.extract()), conf);
				if (!FD_ISSET(cli->socket_fd, &master_write))
					FD_SET(cli->socket_fd, &master_write);
			}
		}
	}
}

size_t	ServerSocketPool::recvRequest(ClientSocket* cli, int& retflags)
{
	size_t total = 0;
	char buf[MAXBUF + 1];
	*buf = 0;

	retflags = 0;

	// - recv() will return -1 and set EAGAIN if socket is non-blocking and no data available
	// - recv() returns 0 if client sends ctrl-c (probably sets EINTR too)

	// buffers the bytes read across calls, then set ready when crlf is found
	int ret;
	while ( (ret = recv(cli->socket_fd, buf, MAXBUF, 0)) > 0 )
	{
		retflags |= (int)IOSTATE::ONCE;
		buf[ret] = 0;
		total += ret;
		cli->req_buffer.append(buf, ret);
		if (cli->req_buffer.ready())
		{
			retflags |= (int)IOSTATE::READY;
			break;
		}
	}

	return total;
}

void	ServerSocketPool::pollWrite(Socket* s)
{
	Logger& log = Logger::getInstance();

	ClientSocket* cli = static_cast<ClientSocket*>(s);

	// For all active http exchange tickets, send what is possible to send
	while (!cli->exchanges.empty())
	{
		int retflags = 0;
		size_t sendbytes = sendResponse(cli, retflags);
		log.out() << "[outbound]: "
					<< "fd="  << cli->socket_fd << ", "
					<< "size=" << sendbytes << std::endl;
		
		if (retflags & (int)IOSTATE::READY)
		{
			// Full response has been sent
			std::string msg(cli->getExchange().response_headers.str());
			if (msg.find("\r\n\r\n") != std::string::npos)
				msg = msg.substr(0, msg.find("\r\n\r\n"));
			log.out() << "[response]: fd=" << cli->socket_fd << std::endl;
			log.out(msg);

			cli->closeExchange();
		}
		else
		{
			// Full response has not been sent yet, break out
			if (!(retflags & (int)IOSTATE::ONCE))
			{
				// Socket on write poll but no bytes sent, clear from write poll
				FD_CLR(cli->socket_fd, &master_write);
			}
			else if (!cli->getExchange().end)
				request_handler(cli->getExchange(), conf);
			break;
		}
	}

	if (cli->exchanges.empty())
		FD_CLR(cli->socket_fd, &master_write);
}


size_t	ServerSocketPool::sendResponse(ClientSocket* cli, int& retflags)
{
	size_t total = 0;
	ResponseBuffer& response_buf = *cli->getExchange().response_buffer;

	retflags &= 0;

	ssize_t ret;
	size_t sendbytes = std::min(response_buf.get().size(), (size_t)MAXBUF);
	while ((ret = send(cli->socket_fd, response_buf.get().get(), sendbytes, MSG_NOSIGNAL)) > 0)
	{
		response_buf.advance(ret);
		retflags |= (int)IOSTATE::ONCE;
		total += ret;
		sendbytes = std::min(response_buf.get().size(), (size_t)MAXBUF);
		if (response_buf.eof())
		{
			if (cli->getExchange().end)
				retflags |= (int)IOSTATE::READY;
			break;
		}
	}
	return total;
}
