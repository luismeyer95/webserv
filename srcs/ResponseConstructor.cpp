#include <ResponseConstructor.hpp>

ResponseConstructor::ResponseConstructor()
    : _first_line("HTTP/1.1 "), _code(""), _error(0),
        _content_length(""), _content_location(""),
        _date(""), _last_modified(""), _location(""), _retry_after(""),
        _server("Server: Webserv/1.0 (Unix)\r\n"), _www_authenticate("")
{

}

ResponseConstructor::~ResponseConstructor()
{
    
}

ByteBuffer ResponseConstructor::constructor(RequestParser &req, FileRequest &file_request)
{
    _error = file_request.http_code;
    _code = get_http_code(file_request.http_code);
    _first_line.append(_code + "\r\n");//first line HTTP/1.1 + code


    if (req.getMethod() == "GET" || req.getMethod() == "POST")
    {
        _header << _first_line;
        _header << date();
        _header << last_modified(file_request);
        _header << _server;
        content_length(file_request);
        if (_error == 401)
            _header << www_authenticate(file_request);
        if (_error == 301 || _error == 503)
            _header << retry_after();
        _header << "\r\n";
        _header.append(file_request.file_content);
    }
    else if (req.getMethod() == "POST")
    {

    }
    else if (req.getMethod() == "HEAD")
    {

    }
    else if (req.getMethod() == "PUT")
    {
        
    }
    else if (req.getMethod() == "DELETE")
    {
        
    }
    else if (req.getMethod() == "CONNECT")
    {
        
    }
    else if (req.getMethod() == "OPTIONS")
    {
        
    }
    else if (req.getMethod() == "TRACE")
    {
        
    }

    return (_header);
}

std::string ResponseConstructor::date()
{
    _date = "Date: ";
    _date.append(get_gmt_time(time(0)));
    _date.append("\r\n");
    return (_date);
}

std::string ResponseConstructor::retry_after()
{
    //sent with 503, 429, 301
    _retry_after = "Retry-After: ";
    _retry_after.append("120");
    _retry_after.append("\r\n");
    return (_retry_after);
}

std::string ResponseConstructor::www_authenticate(FileRequest file_request)
{
    _www_authenticate = "WWW-Authenticate: Basic ";
    _www_authenticate.append("realm=");
    _www_authenticate.append(file_request.realm);
    _www_authenticate.append("\r\n");
    return (_www_authenticate);
}

std::string ResponseConstructor::last_modified(FileRequest file_request)
{
    _last_modified = "Last-Modified: ";
    _last_modified.append(file_request.last_modified);
    _last_modified.append("\r\n");
    return (_last_modified);
}

void ResponseConstructor::content_length(FileRequest file_request)
{
    _header << "Content-Length: " << file_request.file_content.size() << "\r\n";
}