#include "../includes/Utils.hpp"
#include <Requestparser.hpp>

RequestParser::RequestParser()
    : _method(""), _resource(""), _protocol("")
    
{
    _headers.push_back("Accept-Charset");
    _headers.push_back("Accept-Language");
    _headers.push_back("Allow");
    _headers.push_back("Authorization");
    _headers.push_back("Content-Language");
    _headers.push_back("Content-Length");
    _headers.push_back("Content-Location");
    _headers.push_back("Content-Type");
    _headers.push_back("Date");
    _headers.push_back("Host");
    _headers.push_back("Referer");
    _headers.push_back("User-Agent");
}

RequestParser::~RequestParser()
{
    
}

int RequestParser::parser(const std::string header)
{
    _temp = tokenizer(header, '\n');

    if (tokenizer(_temp[0], ' ').size() != 3)
        return (1);
    
    _method = tokenizer(_temp[0], ' ')[0];
    _resource = tokenizer(_temp[0], ' ')[1];
    _protocol = tokenizer(_temp[0], ' ')[2];

    accept_charset_parser(_temp);
    accept_language_parser(_temp);
    allow_parser(_temp);
    authorization_parser(_temp);
    content_language_parser(_temp);
    content_length_parser(_temp);
    content_location_parser(_temp);
    content_type_parser(_temp);
    date_parser(_temp);
    host_parser(_temp);
    referer_parser(_temp);
    user_agent_parser(_temp);
    
    return (0);
}

void RequestParser::accept_charset_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    
    line = header_finder(head, "Accept-Charset");
    if (line.max_size() == 0)
        return;
}

void RequestParser::accept_language_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    
    line = header_finder(head, "Accept-Language");
    if (line.max_size() == 0)
        return;
}

void RequestParser::allow_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    
    line = header_finder(head, "Allow");
    if (line.max_size() == 0)
        return;
}

void RequestParser::authorization_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    
    line = header_finder(head, "Authorization");
    if (line.max_size() == 0)
        return;
}

void RequestParser::content_language_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    
    line = header_finder(head, "Content-language");
    if (line.max_size() == 0)
        return;
}

void RequestParser::content_length_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    
    line = header_finder(head, "Content-Length");
    if (line.max_size() == 0)
        return;
    if (line.size() == 2)
    {
        if (!is_number(line[1]))
            ;//error 400 ? + compare to max client size
        _content_length = line[1];
    }
}

void RequestParser::content_location_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    
    line = header_finder(head, "Content-Location");
    if (line.max_size() == 0)
        return;
    if (line.size() == 2)
        _content_location = line[1];
}

void RequestParser::content_type_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    
    line = header_finder(head, "Content-Type");
    if (line.max_size() == 0)
        return;
}

void RequestParser::date_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    
    line = header_finder(head, "Date");
    if (line.max_size() == 0)
        return;
    if (line.size() == 2 && is_http_date(trim(line[1])))
        _date = trim(line[1]);
}

void RequestParser::host_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    
    line = header_finder(head, "Host");
    if (line.max_size() == 0)
        return;
    if (line.size() == 2)
    {
        if (tokenizer(line[1], ':').size() > 2)
            ;//check domain name syntax + error 400 ? + if 2 Host header
        _host = line[1];
    }
}

void RequestParser::referer_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    
    line = header_finder(head, "Referer");
    if (line.max_size() == 0)
        return;
    if (line.size() == 2)
        //check URL syntax
        _referer = line[1];
}

void RequestParser::user_agent_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    
    line = header_finder(head, "User-Agent");
    if (line.max_size() == 0)
        return;
}
