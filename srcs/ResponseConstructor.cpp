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

}