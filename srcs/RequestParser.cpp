#include <Utils.hpp>
#include <RequestParser.hpp>
#include <Regex.hpp>

RequestParser::RequestParser()
    : _method(""), _resource(""), _protocol("") //penser à setup
    
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
    std::vector<std::string> temp;
    temp = tokenizer(header, '\n');

    if (tokenizer(temp[0], ' ').size() != 3)
        return (1);
    
    _method = tokenizer(temp[0], ' ')[0];//check if Method no allowed
    _resource = tokenizer(temp[0], ' ')[1];
    _protocol = tokenizer(temp[0], ' ')[2];

    accept_charset_parser(temp);
    accept_language_parser(temp);
    allow_parser(temp);
    authorization_parser(temp);
    content_language_parser(temp);
    content_length_parser(temp);
    content_location_parser(temp);
    content_type_parser(temp);
    date_parser(temp);
    host_parser(temp);
    referer_parser(temp);
    user_agent_parser(temp);
    
    return (0);
}

void RequestParser::accept_charset_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    std::vector<std::string> tmp;
    std::vector<int> w;
    std::string tmp2;
    Regex weight("q=[0-9]\\.[0-9]( )*,");
    Regex num("[0-9]\\.[0-9]");

    line = header_finder(head, "Accept-Charset");
    if (line.max_size() == 0)
        return;
    line = tokenizer(line[1], ',');
    for (std::vector<std::string>::iterator it = line.begin(); it != line.end(); it++)
    {
        tmp = tokenizer(*it, ';');
        if (tmp.size() > 2)
            return ((void)_accept_charset.clear());
        if (tmp.size() == 2 && weight.match(tmp[1]).first)
        {
            tmp2 = num.match(weight.match(tmp[1]).second).second;
            tmp = tokenizer(tmp2, '.');
            tmp2.clear();
            tmp2.append(tmp[0]);
            tmp2.append(tmp[1]);
            w.push_back(atoi(tmp2.c_str()));
        }
        else
            w.push_back(1);
         _accept_charset.push_back(trim(line[0]));
    }
    for (unsigned long i = 0; i < _accept_charset.size() - 1; i++)
    {
        if (w[i] > w[i + 1])
        {
            w.push_back(w[i]);
            w.erase(w.begin() + i);
            _accept_charset.push_back(_accept_charset[i]);
            _accept_charset.erase(_accept_charset.begin() + i);
            i = 0;
        }
    }
}

void RequestParser::accept_language_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    std::vector<std::string> tmp;
    std::vector<int> w;
    std::string tmp2;
    Regex weight("q=[0-9]\\.[0-9]( )*,");
    Regex num("[0-9]\\.[0-9]");
    
    line = header_finder(head, "Accept-Language");
    if (line.max_size() == 0)
        return;
    line = tokenizer(line[1], ',');
    for (std::vector<std::string>::iterator it = line.begin(); it != line.end(); it++)
    {
        tmp = tokenizer(*it, ';');
        if (tmp.size() > 2)
            return ((void)_accept_language.clear());
        if (tmp.size() == 2 && weight.match(tmp[1]).first)
        {
            tmp2 = num.match(weight.match(tmp[1]).second).second;
            tmp = tokenizer(tmp2, '.');
            tmp2.clear();
            tmp2.append(tmp[0]);
            tmp2.append(tmp[1]);
            w.push_back(atoi(tmp2.c_str()));
        }
        else
            w.push_back(1);
         _accept_language.push_back(trim(line[0]));
    }
    for (unsigned long i = 0; i < _accept_language.size() - 1; i++)
    {
        if (w[i] > w[i + 1])
        {
            w.push_back(w[i]);
            w.erase(w.begin() + i);
            _accept_language.push_back(_accept_language[i]);
            _accept_language.erase(_accept_language.begin() + i);
            i = 0;
        }
    }
}

void RequestParser::allow_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    std::vector<std::string> req_methods = {"GET", "HEAD", "POST", "PUT", "DELETE", "CONNECT", "OPTIONS", "TRACE"};

    line = header_finder(head, "Allow");
    if (line.max_size() == 0)
        return;
    line = tokenizer(line[1], ',');
     for (std::vector<std::string>::iterator it = line.begin(); it != line.end(); it++)
     {
         for (unsigned long i = 0; i < req_methods.size(); i++)
         {
             if (trim(*it) == req_methods[i])
                break;
            if (i == req_methods.size() - 1)//if method does not exist what to do ?
                return;
         }
         _allow.push_back(trim(*it));
     }
}

void RequestParser::authorization_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    std::vector<std::string> tmp;
    
    line = header_finder(head, "Authorization");
    if (line.max_size() == 0)
        return;
    line = tokenizer(line[1], ' ');
    if (line.size() != 2)
        return;
    if (trim(line[0]) == "Basic")
    {
        _authorization.method = line[0];
        _authorization.basic = line[1];
    }
    else if (trim(line[0]) == "Digest")
    {
        _authorization.method = line[0];
        line = tokenizer(line[1], ',');
        for (std::vector<std::string>::iterator it = line.begin(); it != line.end(); it++)
        {
            tmp = tokenizer(*it, '=');
            if (tmp.size() != 2)
                return;
            if (tmp[0] == "realm" && _authorization.realm == "")
                _authorization.realm = tmp[2];
            else if (tmp[0] == "username" && _authorization.username == "")
                _authorization.username = tmp[2];
            else if (tmp[0] == "domain" && _authorization.domain == "")
                _authorization.domain = tmp[2];
            else if (tmp[0] == "nonce" && _authorization.nonce == "")
                _authorization.nonce = tmp[2];
            else if (tmp[0] == "opaque" && _authorization.opaque == "")
                _authorization.opaque = tmp[2];
            else if (tmp[0] == "algorithm" && _authorization.algorithm == "")
                _authorization.algorithm = tmp[2];
            else if (tmp[0] == "uri" && _authorization.uri == "")
                _authorization.uri = tmp[2];
            else if (tmp[0] == "qop" && _authorization.qop == "")
                _authorization.response = tmp[2];
            else if (tmp[0] == "nc" && _authorization.nc == "")
                _authorization.nc = tmp[2];
            else if (tmp[0] == "cnonce" && _authorization.cnonce == "")
                _authorization.cnonce = tmp[2];
            else
                return;
        }
        //calcul ?
    }
    else
        return;
}

void RequestParser::content_language_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    Regex language("\\A[0-9A-Za-z-]*$");

    line = header_finder(head, "Content-language");
    if (line.max_size() == 0)
        return;
    line = tokenizer(line[1], ',');
    for (std::vector<std::string>::iterator it = line.begin(); it != line.end(); it++)
    {
        if (language.match(trim(*it)).first)
            _content_language.push_back(trim(*it));
        //else Error ?
    }
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
        _content_length = atoi(line[1].c_str());
    }
}

void RequestParser::content_location_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    
    line = header_finder(head, "Content-Location");
    if (line.max_size() == 0)
        return;
    if (line.size() == 2)
        _content_location = line[1]; //CHECK URL SYNTAX
}

void RequestParser::content_type_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    std::vector<std::string> tmp;
    
    line = header_finder(head, "Content-Type");
    if (line.max_size() == 0)
        return;
    line = tokenizer(line[1], ';');
    tmp = tokenizer(line[0], '/');
    if (tmp.size() != 2)
        return;
    _content_type.media_type = tmp[0];
    _content_type.subtype = tmp[1];
    for (unsigned int i = 1; i < line.size(); i++)
    {
        tmp = tokenizer(line[i], '=');
        if (tmp.size() != 2)
            return; //error ?
        if (tmp[0] == " charset")
            _content_type.charset = tmp[1];
        else if (tmp[0] == " boundary")
            _content_type.boundary = tmp[1];
        else
            return; //error ?
    }
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
            ;//check domain name syntax + port + error 400 + if 2 Host header + if no header host
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
    std::vector<std::string> tmp;
    
    line = header_finder(head, "User-Agent");
    if (line.max_size() == 0)
        return;
    tmp = tokenizer(line[1], ' ');
    tmp = tokenizer(trim(tmp[0]), '/');
    if (tmp.size() != 2)
        return;
    _user_agent.product = tmp[0];
    _user_agent.version = tmp[1];
    tmp = tokenizer(line[1], ' ');
    for (std::vector<std::string>::iterator it = tmp.begin() + 1; it != tmp.end(); it++)//decripter ?
    {
        _user_agent.comment.append(*it);
        if (it != tmp.end() - 1)
            _user_agent.comment.push_back(' ');
    }
}
