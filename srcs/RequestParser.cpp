#include <Utils.hpp>
#include <RequestParser.hpp>
#include <Regex.hpp>
#include <URL.hpp>

RequestParser::RequestParser()
    : _error(0), _method(""), _resource(""), _protocol(""),
       _authorization(""), _content_length(0), _content_location(""),
        _date(""), _host_name(""), _host_ip(0), 
        _referer("")
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

int RequestParser::parser(const ByteBuffer request)
{
    std::vector<std::string> temp;
    std::string header;

    header = request.sub(0, request.find({'\r','\n','\r','\n'})).str();
    _payload = request.sub(request.find({'\r','\n','\r','\n'}) + 4);
    
    temp = strsplit(header, "\n");
    
    if (strsplit(temp[0], " ").size() != 3)
        return (1);
    
    _method = strsplit(temp[0], " ")[0];//check if Method no allowed
    try
    {
		_resource = strsplit(temp[0], " ").at(1);
    	URL url(_resource);
    }
    catch(const std::exception& e)
    {
        _error = 400;
        return (1);
    }

    _protocol = strsplit(temp[0], " ")[2];
    _protocol = strsplit(_protocol, "\r")[0];
    if (_protocol != "HTTP/1.1")
    {
        _error = 505;
        return (1);
    }

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
    _raw_accept_charset = line[1];
    line = strsplit(line[1], ",");
    for (std::vector<std::string>::iterator it = line.begin(); it != line.end(); it++)
    {
        tmp = strsplit(*it, ";");
        if (tmp.size() > 2)
            return ((void)_accept_charset.clear());
        if (tmp.size() == 2 && weight.match(tmp[1]).first)
        {
            tmp2 = num.match(weight.match(tmp[1]).second).second;
            tmp = strsplit(tmp2, ".");
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
    _raw_accept_language = line[1];
    line = strsplit(line[1], ",");
    for (std::vector<std::string>::iterator it = line.begin(); it != line.end(); it++)
    {
        tmp = strsplit(*it, ";");
        if (tmp.size() > 2)
            return ((void)_accept_language.clear());
        if (tmp.size() == 2 && weight.match(tmp[1]).first)
        {
            tmp2 = num.match(weight.match(tmp[1]).second).second;
            tmp = strsplit(tmp2, ".");
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
    line = strsplit(line[1], ",");
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
    line = strsplit(line[1], " ");
    if (line.size() != 2)
        return;
    if (trim(line[0]) == "Basic")
        _authorization = line[1];
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
    _raw_content_language = line[1];
    line = strsplit(line[1], ",");
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
    _raw_content_type = line[1];
    line = strsplit(line[1], ";");
    tmp = strsplit(line[0], "/");
    if (tmp.size() != 2)
        return;
    _content_type.media_type = tmp[0];
    _content_type.subtype = tmp[1];
    for (unsigned int i = 1; i < line.size(); i++)
    {
        tmp = strsplit(line[i], "=");
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

    int j = 0;
    for (std::vector<std::string>::iterator it = head.begin(); it != head.end(); it++)
    {
        if (strsplit(*it, ":")[0] == "Host")
            j++;
    }
    if (j != 1)
    {
        _error = 400;
        return;
    }
    line = header_finder(head, "Host");
    if (line.max_size() == 0)
    {
        _error = 400;
        return;
    }
    if (line.size() == 2)
    {
        if (strsplit(line[1], ":").size() > 2)
        {
            _error = 400;
            return;
        }
        if (strsplit(line[1], ":").size() == 2)
        {
            line = strsplit(line[1], ":");
            if (!is_number(line[1]))
            {
                _error = 400;
                return;
            }
            _host_name = line[0];
            _host_ip = (unsigned short)atoi(line[1].c_str());
        }
        else
            _host_name = line[1];
    }
}

void RequestParser::referer_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    
    line = header_finder(head, "Referer");
    if (line.max_size() == 0)
        return;
    if (line.size() == 2)
    {
		std::string tmp = line.at(1);
        try
        {
            URL url(tmp);
            _referer = tmp;
        }
        catch(const std::exception& e)
        {
            return;
        }
    }
}

void RequestParser::user_agent_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    std::vector<std::string> tmp;
    
    line = header_finder(head, "User-Agent");
    if (line.max_size() == 0)
        return;
    _raw_user_agent = line[1];
    tmp = strsplit(line[1], " ");
    tmp = strsplit(trim(tmp[0]), "/");
    if (tmp.size() != 2)
        return;
    _user_agent.product = tmp[0];
    _user_agent.version = tmp[1];
    tmp = strsplit(line[1], " ");
    for (std::vector<std::string>::iterator it = tmp.begin() + 1; it != tmp.end(); it++)
    {
        _user_agent.comment.append(*it);
        if (it != tmp.end() - 1)
            _user_agent.comment.push_back(' ');
    }
}