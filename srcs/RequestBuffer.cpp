#include <RequestBuffer.hpp>
#include <ServerSocketPool.hpp>
#include <Sockets.hpp>

const size_t default_max_body_size = 31457280; // 30MB
const size_t default_max_header_size = 10000;

RequestBuffer::RequestBuffer(RequestRouter& route, ClientSocket* sock)
	: route(route), socket(sock), max_body(default_max_body_size),
	header_break(-1), content_length(0), errcode(-1) {}

template <typename T>
bool				RequestBuffer::isSet(T var)
{
	return var != -1;
}

ssize_t				RequestBuffer::headerBreak()
{
	return request_buffer.strfind("\r\n\r\n");
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
	if (!isSet(header_break) && request_buffer.size() + len > maxRequestLength())
	{
		request_buffer.append((BYTE*)buf, maxRequestLength() - request_buffer.size());
		if (!isSet(headerBreak()))
		{
			errcode = 431;
			processRequest();
		}
		else
			processHeader();
	}
	else if (isSet(header_break) && request_buffer.size() + len > neededLength())
	{
		request_buffer.append((BYTE*)buf, neededLength() - request_buffer.size());
		processRequest();
	}
	else
	{
		request_buffer.append((BYTE*)buf, len);
		if (isSet(headerBreak()) && !isSet(header_break))
			processHeader();
	}
}

bool				RequestBuffer::headerSizeError()
{
	if (static_cast<size_t>(header_break) > default_max_header_size)
	{
		errcode = 431;
		processRequest();
		return true;
	}
	return false;
}

bool				RequestBuffer::parserError()
{
	if (req_parser.getError())
	{
		errcode = req_parser.getError();
		processRequest();
		return true;
	}
	return false;
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


void	RequestBuffer::processHeader()
{
	header_break = headerBreak() + 4;
	req_parser.parser(request_buffer.sub(0, header_break));

	if (processError(static_cast<size_t>(header_break) > default_max_header_size, 431))
		return;
	if (processError(req_parser.getError(), req_parser.getError()))
		return;

	URL url(req_parser.getResource());
	std::string request_path = URL::decode(URL::reformatPath(url.get(URL::Component::Path)));

	route.bindServer(req_parser.getHost(), socket->lstn_socket->address_str, socket->lstn_socket->port);
	bool located = route.bindLocation(request_path);
	if (located)
	{
		auto max_req_body = route.getBoundRequestDirectiveValues(DirectiveKey::max_request_body);
		if (!max_req_body.empty())
			max_body = std::stoull(max_req_body.at(0));
	}

	content_length = req_parser.getContentLength();

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

void				RequestBuffer::processRequest()
{
	FileRequest file_request;

	// std::cout << "Processed request buffer: ";
	// http_print(request_buffer.str());

	HTTPExchange& ticket = socket->newExchange(request_buffer);
	if (isSet(errcode))
		route.fetchErrorPage(file_request, req_parser, errcode, get_http_string(errcode));
	else
	{
		req_parser.getPayload() = request_buffer.sub(header_break);
		file_request = route.requestFile(req_parser, ticket);
	}

	// std::cout << "Processed payload: " << req_parser.getPayload() << std::endl;

	ResponseConstructor response;
	ByteBuffer headers = response.constructor(req_parser, file_request);

	SharedPtr<ResponseBuffer> response_buffer(file_request.response_buffer);
	response_buffer->get().prepend(headers);
	// Load the response in the http exchange ticket and mark as ready
	ticket.bufferResponse(headers, response_buffer, true);

	processed = true;
}

bool				RequestBuffer::ready() const
{
	return processed;
}
