#include <ResponseConstructor.hpp>

ResponseConstructor::ResponseConstructor()
    : _first_line("HTTP/1.1 "), _code(""), _server("Webserv/1.0 (Unix)"),
        _content_length(""), _content_location(""),
        _date(""), _last_modified(""), _location(""), _retry_after(""),
        _www_authenticate("")//finish setup
{

}

ResponseConstructor::~ResponseConstructor()
{
    
}

void ResponseConstructor::constructor(RequestParser req)
{
    FileRequest file_request;

    if (req.getMethod() == "GET")
    {

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


    if (req.getError() != 0)
        _code = get_http_code(req.getError());
    else
        _code = get_http_code(file_request.http_code);

        _first_line.append(_code + "\r\n");//first line HTTP/1.1 + code
}

void ResponseConstructor::date()
{
    _date = "Date: ";
    _date.append(get_gmt_time(time(0)));
    _date.append("\r\n");
}

void ResponseConstructor::retry_after()
{
    //sent with 503, 429, 301
    _retry_after = "Retry-After: ";
    _retry_after.append("120");
    _retry_after.append("\r\n");
}

void ResponseConstructor::www_authenticate(FileRequest file_request)
{
    _www_authenticate = "WWW-Authenticate: Basic ";
    _www_authenticate.append("realm=");//add realm="HERE"
    _www_authenticate.append("\r\n");
}

void ResponseConstructor::last_modified(FileRequest file_request)
{
    _last_modified = "Last-Modified: ";
    _last_modified.append(file_request.last_modified);
    _last_modified.append("\r\n");
}