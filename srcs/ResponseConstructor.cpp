#include <Utils.hpp>
#include <ResponseConstructor.hpp>
#include <Regex.hpp>

ResponseConstructor::ResponseConstructor()
    : _header("HTTP/1.1 "), _code(""), _server("Webserv/1.0 (Unix)")//penser à setup
    
{

}

ResponseConstructor::~ResponseConstructor()
{
    
}

void ResponseConstructor::constructor(RequestParser req)
{
    _header.append(_code + "\r\n");//first line HTTP/1.1 + code

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
}

void ResponseConstructor::date()
{
    char buf[1000];
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S %Z", &tm);
    _date = "Date: ";
    _date.append(buf);
}

void ResponseConstructor::retry_after()
{
    //sent with 503, 429, 301
    _retry_after = "Retry-After: ";
    _retry_after.append("120");
}

void ResponseConstructor::www_authenticate()
{
    _www_authenticate = "WWW-Authenticate: Basic ";
    _www_authenticate.append("realm=");//add realm="HERE"
}