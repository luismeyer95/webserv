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
	: fd_max(-1), current_request(-1)
{
	
}

void	ServerSocketPool::setConfig(RequestRouter conf)
{
	typedef std::pair<std::string, unsigned short> hp_pair;
	ConfBlockDirective& main = *conf.main;
	ft::set<hp_pair> added_hostports;
	this->conf = conf;

	for (auto& b : main.blocks)
	{
		if (b.key == ContextKey::server)
		{
			std::string host_port = RequestRouter::getDirective(b, DirectiveKey::listen).values.at(0);
			// auto tokens = tokenizer(host_port, ':');
			auto tokens = strsplit(host_port, ":");
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

	listen(lstn->socket_fd, 200);
	
	if (lstn->socket_fd > fd_max)
		fd_max = lstn->socket_fd;
}

ServerSocketPool::~ServerSocketPool()
{
	for (auto it = socket_list.begin(); it != socket_list.end(); ++it)
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

std::vector<Socket*>&		ServerSocketPool::getSocketList()
{
	return socket_list;
}

void handle_sigint(int sig) 
{
	(void)sig;
	write(1, "\n", 1);
}

void	ServerSocketPool::runServer()
{
	initFdset();
	signal(SIGINT, handle_sigint);
	while (true)
	{
		fd_set copy_read = master_read;
		fd_set copy_write = master_write;
		int socket_count = select(fd_max + 1, &copy_read, &copy_write, nullptr, nullptr);
		if (socket_count == -1 && errno == EINTR)
			break;

		auto select_list = socket_list;
		for (auto sock : select_list)
		{
			if (selected(sock, &copy_write))
			{
				bool closed = pollWrite(sock);
				if (closed)
					continue;
			}
			if (selected(sock, &copy_read))
				pollRead(sock);
		}
	}
}

bool				ServerSocketPool::enqueue(Socket *sock)
{
	if (current_request == -1)
		current_request = sock->socket_fd;
	else if (current_request != sock->socket_fd)
		return false;
	return true;
}

void				ServerSocketPool::dequeue(Socket *sock)
{
	if (current_request == sock->socket_fd)
		current_request = -1;
}

void	ServerSocketPool::initFdset()
{
	FD_ZERO(&master_read);
	FD_ZERO(&master_write);
	for (auto it = socket_list.begin(); it != socket_list.end(); ++it)
		FD_SET((*it)->socket_fd, &master_read);
}

bool	ServerSocketPool::selected(Socket* socket, fd_set* set)
{
	return FD_ISSET(socket->socket_fd, set);
}

ClientSocket*	ServerSocketPool::acceptConnection(Listener* lstn)
{
	ClientSocket* comm = new ClientSocket(conf);
	comm->lstn_socket = lstn;

	char ipstr[INET_ADDRSTRLEN];

	struct sockaddr store;
	socklen_t		len = sizeof(store);

	comm->socket_fd = accept(lstn->socket_fd, &store, &len);
	struct sockaddr_in *s = (struct sockaddr_in*)&store;
	inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof(ipstr));
	comm->client_address = ipstr;

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
	auto& commlist = comm->lstn_socket->comm_sockets;
	for (auto it = commlist.begin(); it != commlist.end(); ++it)
	{
		if (*it == comm)
		{
			commlist.erase(it);
			break;
		}
	}
	for (auto it = socket_list.begin(); it != socket_list.end(); ++it)
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
		acceptConnection((Listener*)s);
	}
	else 
	{
		if (!enqueue(s))
			return;
		ClientSocket* cli = static_cast<ClientSocket*>(s);
		int retflags = 0;
		recvRequest(cli, retflags);
		if (!(retflags & (int)IOSTATE::ONCE))
		{
			log.out() << "<disconnect fd=" << cli->socket_fd  << ">" << std::endl;
			dequeue(cli);
			closeComm(cli);
		}
		else
		{
			if (retflags & (int)IOSTATE::READY)
			{
				RequestBuffer& buff = cli->req_buffer;
				ByteBuffer msg(buff.get());
				if (msg.strfind("\r\n\r\n") != -1)
					msg = msg.sub(0, msg.strfind("\r\n\r\n"));
				log.out() << "[request]: fd=" << cli->socket_fd << std::endl;
				log.out(msg.str());
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

	ssize_t ret;
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

bool	ServerSocketPool::pollWrite(Socket* s)
{
	Logger& log = Logger::getInstance();
	ClientSocket* cli = static_cast<ClientSocket*>(s);
	while (!cli->exchanges.empty())
	{
		int retflags = 0;
		sendResponse(cli, retflags);
		if (retflags & (int)IOSTATE::READY)
		{
			std::string msg(cli->getExchange().response_headers.str());
			if (msg.find("\r\n\r\n") != std::string::npos)
				msg = msg.substr(0, msg.find("\r\n\r\n"));
			log.out() << "[response]: fd=" << cli->socket_fd << std::endl;
			log.out(msg);
			cli->closeExchange();
		}
		else
		{
			if (!(retflags & (int)IOSTATE::ONCE))
				FD_CLR(cli->socket_fd, &master_write);
			break;
		}
	}
	if (cli->exchanges.empty())
	{
		FD_CLR(cli->socket_fd, &master_write);
		log.out() << "<disconnect fd=" << cli->socket_fd  << ">" << std::endl;
		dequeue(cli);
		closeComm(cli);
		return true;
	}
	return false;
}


size_t	ServerSocketPool::sendResponse(ClientSocket* cli, int& retflags)
{
	size_t total = 0;
	ResponseBuffer& response_buf = *cli->getExchange().response_buffer;

	retflags &= 0;

	ssize_t ret;
	size_t sendbytes = std::min(response_buf.get().size(), (size_t)MAXBUF);
	ByteBuffer& bb = response_buf.get();
	while ((ret = send(cli->socket_fd, bb.get(), sendbytes, MSG_NOSIGNAL)) > 0)
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
		response_buf.get();
	}
	return total;
}
