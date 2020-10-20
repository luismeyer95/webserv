#include <RequestBuffer.hpp>
#include <ServerSocketPool.hpp>
#include <Sockets.hpp>

const size_t default_max_body_size = 100457280; // ~100Mb
const size_t default_max_header_size = 1000000;

RequestBuffer::RequestBuffer(RequestRouter& route, ClientSocket* sock)
	: route(route), socket(sock), max_body(default_max_body_size),
	header_break(-1), content_length(0), errcode(-1), processed(false), chunked_flag(false),
	chunk_eof(false), chunked_body_len(0)
{
}

template <typename T>
bool				RequestBuffer::isSet(T var)
{
	return var != -1;
}

ssize_t				RequestBuffer::headerBreak(ByteBuffer& bb)
{
	return bb.strfind("\r\n\r\n");
}

size_t				RequestBuffer::neededLength()
{
	if (isSet(header_break))
		return header_break + content_length;
	throw std::logic_error("header break is not set");
}

size_t				RequestBuffer::maxRequestLength()
{
	return default_max_header_size + max_body;
}

const ByteBuffer&	RequestBuffer::get() const
{
	return request_buffer;
}

void	RequestBuffer::append(char *buf, size_t len)
{
	if (!isSet(header_break))
		readHeader(buf, len);
	else
		readPayload(buf, len);
}

void				RequestBuffer::readHeader(char *buf, size_t len)
{
	// would exceed max
	if (request_buffer.size() + len > default_max_header_size)
	{
		request_buffer.append((BYTE*)buf, default_max_header_size - request_buffer.size());
		if (processError(!isSet(headerBreak(request_buffer)), 431))
			return;
		else
			processHeader();
	}
	else // would not exceed max
	{
		request_buffer.append((BYTE*)buf, len);
		if (isSet(headerBreak(request_buffer)))
		{
			splitHeaderPayload();
			if (processHeader())
				processRequestIfPossible();
		}
	}
}

void	RequestBuffer::splitHeaderPayload()
{
	header_break = headerBreak(request_buffer) + 4;
}

void			RequestBuffer::dechunk()
{
	ssize_t ret;
	while ((ret = processChunk()) && isSet(ret));
	if (!ret)
		chunk_eof = true;
}

ssize_t			RequestBuffer::processChunk()
{
	ssize_t hex_break = chunk_buffer.strfind("\r\n");
	if (!isSet(hex_break))
		return -1;
	std::string hexnumstr = chunk_buffer.sub(0, hex_break).str();
	size_t hexnum = 0;
	hexnum = std::stoull(hexnumstr, nullptr, 16); 
	if (chunk_buffer.size() >= hexnumstr.size() + 2 + hexnum + 2
		&& chunk_buffer.sub(hex_break + 2 + hexnum).strfind("\r\n") == 0)
	{
		request_buffer.append(chunk_buffer.sub(hex_break + 2, hexnum));
		chunk_buffer = chunk_buffer.sub(hex_break + 2 + hexnum + 2);
		// chunk_buffer.advance(hex_break + 2 + hexnum + 2);
		chunked_body_len += hexnum;
		return hexnum;
	}
	else
		return -1;
}

void				RequestBuffer::readPayload(char *buf, size_t len)
{
	if (chunked_flag)
	{
		chunk_buffer.append((BYTE*)buf, len);
		dechunk();
		if (processError(chunked_body_len > max_body, 413))
			return;
		if (chunk_eof)
			processRequest();
	}
	else
	{
		if (request_buffer.size() + len >= neededLength())
		{
			request_buffer.append((BYTE*)buf, neededLength() - request_buffer.size());
			processRequest();
		}
		else
			request_buffer.append((BYTE*)buf, len);
	}
}


bool	RequestBuffer::processHeader()
{
	if (processError(static_cast<size_t>(header_break) > default_max_header_size, 431))
		return false;

	req_parser.parser(request_buffer.sub(0, header_break));
	if (processError(req_parser.getError(), req_parser.getError()))
		return false;

	URL url(req_parser.getResource());
	std::string request_path = URL::decode(URL::reformatPath(url.get(URL::Component::Path)));

	route.bindServer(req_parser.getHost(), socket->lstn_socket->address_str, socket->lstn_socket->port);
	FileRequest file_req; // only there for the bindLocation() dependency
	bool located = route.bindLocation(file_req, req_parser, request_path, req_parser.getMethod());
	if (located)
	{
		auto max_req_body = route.getBoundRequestDirectiveValues(DirectiveKey::max_request_body);
		if (!max_req_body.empty())
			max_body = std::stoull(max_req_body.at(0));
	}

	if (req_parser.getTransferEncoding() == "chunked")
		chunked_flag = true;
	else
		content_length = req_parser.getContentLength();
	
	return true;
}



void	RequestBuffer::processRequestIfPossible()
{
	if (!chunked_flag)
	{
		if (processError(content_length > max_body, 413))
			return;
		else if (!content_length)
			processRequest();
		else if (request_buffer.size() >= neededLength())
		{
			request_buffer = request_buffer.sub(0, neededLength());
			processRequest();
		}
	}
	else
	{
		chunk_buffer = request_buffer.sub(header_break);
		request_buffer = request_buffer.sub(0, header_break);
		dechunk();
		if (processError(chunked_body_len > max_body, 413))
			return;
		else if (chunk_eof)
			processRequest();
	}
}



bool				RequestBuffer::processError(bool expr, int code)
{
	if (expr)
	{
		errcode = code;
		processRequest();
		return true;
	}
	return false;
}

void				RequestBuffer::processRequest()
{
	FileRequest file_request;

	HTTPExchange& ticket = socket->newExchange(request_buffer);
	if (isSet(errcode))
	{
		route.fetchErrorPage(file_request, req_parser, errcode, get_http_string(errcode));
	}
	else
	{
		req_parser.getPayload() = request_buffer.sub(header_break);
		if (chunked_flag)
			req_parser.getContentLength() = req_parser.getPayload().size();
		file_request = route.requestFile(req_parser, ticket);
	}

	ResponseConstructor response;
	ByteBuffer headers = response.constructor(req_parser, file_request);

	SharedPtr<ResponseBuffer> response_buffer(file_request.response_buffer);
	response_buffer->get().prepend(headers);

	ticket.bufferResponse(headers, response_buffer, true);
	
	processed = true;
}

bool				RequestBuffer::ready() const
{
	return processed;
}
