#include <ResponseConstructor.hpp>

ResponseConstructor::ResponseConstructor()
    : _first_line("HTTP/1.1 "), _code(""), _error(0),
        _content_length(""), _content_location(""), _content_type(),
        _date(""), _last_modified(""), _location(""), _retry_after(""),
        _server("Server: Webserv/1.0 (Unix)\r\n"), _www_authenticate("")
{

}

ResponseConstructor::~ResponseConstructor()
{
    
}

ByteBuffer ResponseConstructor::constructor(RequestParser &req, FileRequest &file_request)
{
	(void)req;

    _error = file_request.http_code;
    _code = get_http_code(file_request.http_code);
    _first_line.append(_code + "\r\n");//first line HTTP/1.1 + code

    _header << _first_line;
    date();
    _header << _server;
    content_type(file_request);
    content_length(file_request);
    last_modified(file_request);
	location(file_request);
    allow(file_request);
    www_authenticate(file_request);
    retry_after();
    _header << "\r\n";

    return (_header);
}

void ResponseConstructor::date()
{
    _date = "Date: ";
    _date.append(get_gmt_time(time(0)));
    _date.append("\r\n");
    _header << _date;
}

void ResponseConstructor::retry_after()
{
    if (_error == 503 || _error == 429 || _error == 301)
    {
        _retry_after = "Retry-After: ";
        _retry_after.append("120");
        _retry_after.append("\r\n");
        _header << _retry_after;
    }
}

void ResponseConstructor::www_authenticate(FileRequest& file_request)
{
    if (_error == 401)
    {
        _www_authenticate = "WWW-Authenticate: Basic ";
        _www_authenticate.append("realm=");
        _www_authenticate.append(file_request.realm);
        _www_authenticate.append("\r\n");
        _header << _www_authenticate;
    }
}

void ResponseConstructor::last_modified(FileRequest& file_request)
{
	if (!file_request.last_modified.empty())
		_header << "Last-Modified: " << file_request.last_modified << "\r\n";
}

void ResponseConstructor::content_length(FileRequest& file_request)
{
	if (file_request.content_length != -1)
    	_header << "Content-Length: " << file_request.content_length << "\r\n";
	else
		_header << "Transfer-Encoding: chunked\r\n";
}

void ResponseConstructor::content_type(FileRequest& file_request)
{
	if (!file_request.content_type.empty())
		_header << "Content-Type: " << file_request.content_type << "\r\n";
}

void ResponseConstructor::location(FileRequest& file_request)
{
	if (_error == 302)
		_header << "Location: " << file_request.redirect_uri << "\r\n";
}

void ResponseConstructor::allow(FileRequest& file_request)
{
    if (_error == 405)
    {
        _allow = "Allow: ";
        for (std::vector<std::string>::iterator it = file_request.allowed_methods.begin(); it != file_request.allowed_methods.end(); it++)
        {
            _allow.append(*it);
            if (it != --file_request.allowed_methods.end())
                _allow.append(", ");
        }
        _allow.append("\r\n");
        _header << _allow;
    }
}