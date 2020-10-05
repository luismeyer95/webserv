#include <ResponseBuffer.hpp>

/*		-------------------
**		class ResponseBuffer;
**		-------------------
*/

ResponseBuffer::ResponseBuffer() {}

ByteBuffer& ResponseBuffer::get(size_t bytes)
{
	(void)bytes;
	return buffer;
}

std::vector<std::string>& ResponseBuffer::getHeaders()
{
	return cgi_headers_buf;
}

bool ResponseBuffer::eof()
{
	return buffer.empty();
}

void ResponseBuffer::advance(size_t bytes)
{
	buffer.advance(bytes);
}

ResponseBuffer::~ResponseBuffer() {}


/*		-------------------
**		class ResponseBufferFileStream;
**		-------------------
*/

ResponseBufferFileStream::ResponseBufferFileStream(const string& filename, bool chunked)
	: in(-1), file_path(filename), eof_flag(false), chunked_flag(chunked)
{
	in = open(filename.c_str(), O_RDONLY | O_NONBLOCK, 0644);
	if (in == -1)
		scriptError("could not open file");
}

void ResponseBufferFileStream::scriptError(const std::string& errlog, bool thrw)
{
	Logger& log = Logger::getInstance();
	log.hl(BOLDRED "CGI Failure", std::string(BOLDWHITE) + file_path + ": " + errlog);
	if (thrw)
		throw ErrorCode(500, "Internal Server Error");
}

ByteBuffer& ResponseBufferFileStream::get(size_t bytes)
{
	if (buffer.size() < bytes)
		readStream(bytes - buffer.size());
	return buffer;
}

bool ResponseBufferFileStream::eof()
{
	return buffer.empty() && eof_flag;
}

void ResponseBufferFileStream::advance(size_t bytes)
{
	buffer = buffer.sub(bytes);
}

void ResponseBufferFileStream::readStream(size_t bytes)
{
	ssize_t ret;

	if (bytes > MAXBUF)
		bytes = MAXBUF;
	char buf[MAXBUF + 1];
	ret = read(in, buf, bytes);
	if (!ret)
	{
		eof_flag = true;
		if (chunked_flag)
			buffer.append((BYTE*)"0\r\n\r\n", 5);
	}
	else if (ret != -1)
	{
		buf[ret] = 0;
		if (chunked_flag)
		{
			auto hexret = ntohexstr(static_cast<size_t>(ret));
			buffer.append((BYTE*)&hexret[0], hexret.size());
			buffer.append((BYTE*)"\r\n", 2);
			buffer.append((BYTE*)buf, ret);
			buffer.append((BYTE*)"\r\n", 2);
		}
		else
			buffer.append((BYTE*)buf, ret);
	}
}

ResponseBufferFileStream::~ResponseBufferFileStream()
{
	close(in);
}

/*		-------------------
**		class ResponseBufferProcessStream;
**		-------------------
*/

void ResponseBufferProcessStream::scriptError(const std::string& errlog, bool thrw)
{
	Logger& log = Logger::getInstance();
	log.hl(BOLDRED "CGI Failure", std::string(BOLDWHITE) + file_path + ": " + errlog);
	if (thrw)
		throw ErrorCode(500, "Internal Server Error");
}

// Constructs a CGI process-output stream buffer
ResponseBufferProcessStream::ResponseBufferProcessStream (
	const string& script_path, const ByteBuffer& request_payload,
	const string& bin, const cstring_vec& argv, const cstring_vec& env, bool chunked)
: ResponseBuffer(), eof_flag(false), chunked_flag(chunked)
{
	file_path = script_path;

	if (pipe(pip_in) == -1 || pipe(pip_out) == -1)
		scriptError(std::string("pipe(): ") + strerror(errno));
	worker_pid = fork();
	if (worker_pid == -1)
		scriptError(std::string("fork(): ") + strerror(errno));
	if (!worker_pid)
		execScript(bin, argv, env);
	else if (worker_pid > 0)
	{
		close(pip_in[1]);
		close(pip_out[0]);
		timer_pid = fork();
		if (timer_pid == -1)
			scriptError(std::string("fork(): ") + strerror(errno));
		if (!timer_pid)
		{
			close(pip_in[0]);
			close(pip_out[1]);

			usleep(TIMEOUT);
			if (kill(-worker_pid, SIGTERM) == -1)
				kill(-worker_pid, SIGKILL);
			exit(0);
		}
		else if (timer_pid > 0)
		{
			feedRequestPayload(request_payload);
			storeHeaders();
		}
	}
}

ResponseBufferProcessStream::~ResponseBufferProcessStream()
{
	reap();
}

bool ResponseBufferProcessStream::eof()
{
	return buffer.empty() && eof_flag;
}

void ResponseBufferProcessStream::advance(size_t bytes)
{
	buffer = buffer.sub(bytes);
}

void	ResponseBufferProcessStream::execScript(const string& bin, const cstring_vec& argv, const cstring_vec& env)
{
	setpgid(0, 0);
	close(2);
	close(pip_out[1]);
	dup2(pip_out[0], 0);
	close(pip_out[0]);
	close(pip_in[0]);
	dup2(pip_in[1], 1);
	close(pip_in[1]);
	execve(bin.c_str(), &argv[0], &env[0]);
	exit(1);
}

void ResponseBufferProcessStream::feedRequestPayload(ByteBuffer request_payload)
{
	size_t		sendbytes = request_payload.size();
	ssize_t		ret = 0;

	while ((ret = write(pip_out[1], request_payload.get(), sendbytes)) > 0)
	{
		request_payload.advance(ret);
		sendbytes = request_payload.size();
	}
	close(pip_out[1]);
}

bool ResponseBufferProcessStream::searchHeaderBreak(ssize_t& break_pos, ssize_t& break_len)
{
	ssize_t double_lf = buffer.strfind("\n\n");
	ssize_t double_crlf = buffer.strfind("\r\n\r\n");

	if (double_crlf == -1 && double_lf == -1)
		return false;

	if (double_lf < double_crlf)
	{
		break_pos = double_lf;
		break_len = 2;
	}
	else
	{
		break_pos = double_crlf;
		break_len = 4;
	}

	return true;
}

void ResponseBufferProcessStream::storeHeaders()
{
	ssize_t ret;
	ssize_t header_break = 0;
	ssize_t header_break_len = 0;

	// reads until header_break is found, chunks the remaining part if non-empty and chunked_flag is true.
	// the rest will be read with get() called directly inside the socket sending function
	char buf[MAXBUF + 1];
	while ((ret = read(pip_in[0], buf, MAXBUF)) > 0)
	{
		buf[ret] = 0;
		buffer.append((BYTE*)buf, ret);
		if (searchHeaderBreak(header_break, header_break_len))
		{
			cgi_headers_buf = strsplit(buffer.sub(0, header_break).str(), "\r\n");
			if (cgi_headers_buf.empty())
				scriptError("no headers found in output of script");
			buffer = buffer.sub(header_break + header_break_len);
			if (chunked_flag && !buffer.empty())
			{
				auto hexlen = ntohexstr(buffer.size()) + "\r\n";
				buffer.prepend((BYTE*)&hexlen[0], hexlen.size());
				buffer.append((BYTE*)"\r\n", 2);
			}
			break;
		}
	}
}

std::vector<std::string>& ResponseBufferProcessStream::getHeaders()
{
	return cgi_headers_buf;
}

ByteBuffer& ResponseBufferProcessStream::get(size_t bytes)
{
	if (buffer.size() < bytes)
		readStream(bytes - buffer.size());
	return buffer;
}

void ResponseBufferProcessStream::readStream(size_t bytes)
{
	ssize_t ret;

	if (bytes > MAXBUF)
		bytes = MAXBUF;
	char buf[MAXBUF + 1];
	ret = read(pip_in[0], buf, bytes);
	if (!ret && !eof_flag)
	{
		eof_flag = true;
		if (chunked_flag)
			buffer.append((BYTE*)"0\r\n\r\n", 5);
	}
	else if (ret && ret != -1)
	{
		buf[ret] = 0;
		if (chunked_flag)
		{
			auto hexret = ntohexstr(static_cast<size_t>(ret));
			// std::cout << "CHUNKING" << std::endl;
			buffer.append((BYTE*)&hexret[0], hexret.size());
			buffer.append((BYTE*)"\r\n", 2);
			buffer.append((BYTE*)buf, ret);
			buffer.append((BYTE*)"\r\n", 2);
		}
		else
			buffer.append((BYTE*)buf, ret);
	}
}

void ResponseBufferProcessStream::reap()
{
	close(pip_in[0]);
	waitpid(WAIT_ANY, &pstatus, 0);
	if (WIFEXITED(pstatus))
	{
		kill(timer_pid, SIGKILL);
		waitpid(timer_pid, nullptr, 0);
		if (WEXITSTATUS(pstatus))
			scriptError("script execution failed", false);
	}
	else
	{
		while (wait(nullptr) != -1);
		scriptError("script execution time-out", false);
	}
}