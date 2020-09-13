#include <Utils.hpp>
#include <ResponseConstructor.hpp>
#include <Regex.hpp>

ResponseConstructor::ResponseConstructor()
    : _header(""), _method(""), _code("")//penser à setup
    
{

}

ResponseConstructor::~ResponseConstructor()
{
    
}

void ResponseConstructor::constructor(RequestParser req)
{
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