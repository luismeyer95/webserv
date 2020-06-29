#include "../includes/ServerSocketPool.hpp"

void	http_print(const std::string& s)
{
	for (size_t i = 0; i < s.size(); ++i)
	{
		if (s[i] == '\n')
			std::cout << "\\n";
		else if (s[i] == '\r')
			std::cout << "\\r";
		else
			std::cout << s[i];
	}
	std::cout << std::endl;
}

void	dec_print(const char *s)
{
	for (size_t i = 0; s[i]; ++i)
		std::cout << (int)s[i] << " ";
	std::cout << std::endl;

}

ServerSocketPool::ServerSocketPool() : fd_max(-1) {}

ServerSocketPool&	ServerSocketPool::getInstance()
{
	static ServerSocketPool pool;
	return pool;
}

std::vector<Socket*>&		ServerSocketPool::getSocketList()
{
	return socket_list;
}

bool	ServerSocketPool::portIsUnused(unsigned short port)
{
	for (iterator it = socket_list.begin(); it != socket_list.end(); ++it)
		if ((*it)->socket_fd == port)
			return false;
	return true;
}

bool	ServerSocketPool::selected(Socket* socket, fd_set* set)
{
	return FD_ISSET(socket->socket_fd, set);
}


void	ServerSocketPool::addListener(unsigned short port)
{
	Logger& log = Logger::getInstance();

	log.err() << "Requested listening port already in use";
	if (!log.assert(portIsUnused(port), false))
		return;
		
	Listener* lstn = new Listener();

	lstn->port = port;

	lstn->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	log.err() << "Failed to create server socket";
	log.assert(lstn->socket_fd != -1, true);

	lstn->address.sin_family = AF_INET;
	lstn->address.sin_port = htons(port);
	lstn->address.sin_addr.s_addr = INADDR_ANY;

	log.err() << "Failed to bind socket to server address";
	log.assert(bind(lstn->socket_fd, (struct sockaddr*)&lstn->address, sizeof(lstn->address)) != -1, true);
	listen(lstn->socket_fd, MAXQUEUE);
	
	socket_list.push_back((Socket*)lstn);
	
	if (lstn->socket_fd > fd_max)
		fd_max = lstn->socket_fd;
}

ClientSocket*	ServerSocketPool::acceptConnection(Listener* lstn)
{
	ClientSocket* comm = new ClientSocket();
	comm->lstn_socket = lstn;
	// MIGHT NEED TO STORE ACCEPT PARAM VALUES LATER
	comm->socket_fd = accept(lstn->socket_fd, nullptr, nullptr);
	fcntl(comm->socket_fd, F_SETFL, O_NONBLOCK);
	comm->req_buffer = "";
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
			delete comm;
			comm = nullptr;
			return;
		}
	}
}


void	ServerSocketPool::initFdset()
{
	FD_ZERO(&master_read);
	FD_ZERO(&master_write);
	for (std::vector<Socket*>::iterator it = socket_list.begin(); it != socket_list.end(); ++it)
		FD_SET((*it)->socket_fd, &master_read);
}

size_t	ServerSocketPool::httpRequestToStr(ClientSocket* cli, int& retflags)
{
	Parser& parse = Parser::getInstance();
	size_t total = 0;
	char buf[MAXBUF + 1];
	*buf = 0;

	retflags = 0;

	// read what you can of the socket.
	// - recv() will return -1 and set EAGAIN if socket is non-blocking and no data available
	// - recv() returns 0 if client sends ctrl-c (probably sets EINTR too)
	// buffer the bytes read across calls, then set ready and send to callback when crlf is found
	int ret;
	while ( (ret = recv(cli->socket_fd, buf, MAXBUF, 0)) > 0 )
	{
		// CHECK FOR POSSIBLE -1 WITHOUT EAGAIN SET
		retflags |= (int)IOSTATE::ONCE;
		buf[ret] = 0;
		total += ret;
		cli->req_buffer.append(buf);
		if (cli->req_buffer.find("\r\n\r\n") != std::string::npos)
		{
			retflags |= (int)IOSTATE::READY;
			break;
		}
	}

	return total;
}

size_t	ServerSocketPool::sendResponse(ClientSocket* cli, int& retflags)
{
	Parser& parse = Parser::getInstance();
	size_t total = 0;
	std::string& response_buf = cli->getExchange().response_buffer;

	retflags &= 0;

	ssize_t ret;
	size_t sendbytes = std::min(response_buf.size() + 1, (size_t)MAXBUF);
	while ( (ret = send(cli->socket_fd, response_buf.c_str(), sendbytes, MSG_NOSIGNAL)) > 0 )
	{
		retflags |= (int)IOSTATE::ONCE;
		total += ret;
		if ((size_t)ret >= response_buf.size())
			response_buf.clear();
		else
			response_buf = response_buf.substr(ret, response_buf.size() - ret);
		sendbytes = std::min(response_buf.size() + 1, (size_t)MAXBUF);
		if (response_buf.empty())
		{
			if (cli->getExchange().end)
				retflags |= (int)IOSTATE::READY;
			break;
		}
	}
	return total;
}

void	ServerSocketPool::pollRead(Socket* s)
{
	Logger& log = Logger::getInstance();

	if (s->isListener())
	{
		log.out() << "[connection]\n";
		ClientSocket* client_socket = acceptConnection((Listener*)s);
		connection_handler(client_socket->newExchange());
		if (!FD_ISSET(client_socket->socket_fd, &master_write))
			FD_SET(client_socket->socket_fd, &master_write);
	}
	else 
	{
		// read up and stringify http message
		// close the socket and remove it from the pool if it didnt read
		ClientSocket* cli = static_cast<ClientSocket*>(s);
		int retflags = 0;
		size_t readbytes = httpRequestToStr(cli, retflags);
		if (!(retflags & (int)IOSTATE::ONCE))
		{
			closeComm(cli);
			log.out() << "<disconnect>" << std::endl;
		}
		else
		{
			log.out() << "[inbound packet]: "
				<< "fd="  << cli->socket_fd << ", "
				<< "size=" << readbytes << std::endl;
			if (retflags & (int)IOSTATE::READY)
			{
				// at least one request is fully buffered:
				// - create exchange
				// - put client fd on write queue
				// - an exchange is done and popped from the queue when
				//	 the request_handler has marked the end of the response
				// - write poller will pop client from the write queue
				//	 once the http exchange pool for this client is empty
				std::string msg(cli->req_buffer);
				if (cli->req_buffer.find("\r\n\r\n") != std::string::npos)
					msg = msg.substr(0, msg.find("\r\n\r\n"));
				log.out() << "[request]: fd=" << cli->socket_fd << std::endl;
				log.out(true, false) << msg << std::endl;
				while (cli->req_buffer.find("\r\n\r\n") != std::string::npos)
					request_handler(cli->newExchange());
				if (!FD_ISSET(cli->socket_fd, &master_write))
					FD_SET(cli->socket_fd, &master_write);
			}
		}
	}
}

void	ServerSocketPool::pollWrite(Socket* s)
{
	Logger& log = Logger::getInstance();

	ClientSocket* cli = static_cast<ClientSocket*>(s);

	while (!cli->exchanges.empty())
	{
		int retflags = 0;
		size_t sendbytes = sendResponse(cli, retflags);
		log.out() << "[outbound packet]: "
					<< "fd="  << cli->socket_fd << ", "
					<< "size=" << sendbytes << std::endl;
		
		if (retflags & (int)IOSTATE::READY)
		{
			std::string msg(cli->getExchange().response);
			if (msg.find("\r\n\r\n") != std::string::npos)
				msg = msg.substr(0, msg.find("\r\n\r\n"));
			log.out() << "[response]: fd=" << cli->socket_fd << std::endl;
			log.out(true, false) << msg << std::endl;

			cli->closeExchange();
		}
		else
		{
			request_handler(cli->getExchange());
			break;
		}
	}

	if (cli->exchanges.empty())
		FD_CLR(cli->socket_fd, &master_write);
}


void	ServerSocketPool::runServer(
	void (*connection_handler)(HTTPExchange&) ,
	void (*request_handler)(HTTPExchange&)
)
{
	this->connection_handler = connection_handler;
	this->request_handler = request_handler;
	initFdset();

	int loopcount = 0;
	while (true)
	{
		fd_set copy_read = master_read;
		fd_set copy_write = master_write;
		int socket_count = select(fd_max + 1, &copy_read, &copy_write, nullptr, nullptr);

		int size = socket_list.size();
		for (int i = 0; i != size; i++)
		{
			Socket* s = socket_list[i];
			if (selected(s, &copy_write))
				pollWrite(s);
			if (selected(s, &copy_read))
				pollRead(s);
			
		}
	}
}