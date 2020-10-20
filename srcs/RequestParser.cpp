#include <Utils.hpp>
#include <RequestParser.hpp>
#include <Regex.hpp>
#include <URL.hpp>

RequestParser::RequestParser()
    : _error(0), _method(""), _resource(""), _protocol(""),
       _authorization(""), _content_length(0), _content_location(""),
        _date(""), _host_name(""), _host_ip(0), 
        _referer(""),
        _req_methods({"GET", "HEAD", "POST", "PUT"})
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
    _headers.push_back("Transfer-Encoding");
    _headers.push_back("User-Agent");
}

RequestParser::~RequestParser()
{
    
}

int RequestParser::reportError(int code)
{
	_error = code;
	return code;
}

int RequestParser::parser(const ByteBuffer request)
{
    std::vector<std::string> temp;
    std::string header;

    header = request.sub(0, request.strfind("\r\n\r\n")).str();
    
    temp = strsplit(header, "\r\n");

	if (header.empty())
		return reportError(400);

	auto first_line_tokens = strsplit(temp.at(0), " ");
    if (first_line_tokens.size() != 3)
        return reportError(400);
    _method = first_line_tokens.at(0);

	if (std::find(_req_methods.begin(), _req_methods.end(), _method) == _req_methods.end())
		return reportError(400);

	_resource = strsplit(temp.at(0), " ").at(1);
    try { URL url(_resource); }
    catch(const std::exception& e)
	{
		return reportError(400);
	}

    _protocol = strsplit(temp.at(0), " ").at(2);
    if (_protocol != "HTTP/1.1")
		return reportError(505);

    for (std::vector<std::string>::iterator it = ++temp.begin(); it != temp.end(); it++)
    {
        if ((*it).find(":") == std::string::npos || (*it).find(":") == (*it).size())
            return (reportError(400));
    }
    try
    {
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
        transfer_encoding(temp);
        custom_headers(temp);
    }catch(const std::exception& e)
    {
        return (_error);
    }

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
    if (line.size() == 0)
        return;
    _raw_accept_charset = line.at(1);
    line = strsplit(line.at(1), ",");
    for (std::vector<std::string>::iterator it = line.begin(); it != line.end(); it++)
    {
        tmp = strsplit(*it, ";");
        if (tmp.size() > 2)
            return ((void)_accept_charset.clear());
        if (tmp.size() == 2 && weight.match(tmp.at(1)).first)
        {
            tmp2 = num.match(weight.match(tmp.at(1)).second).second;
            tmp = strsplit(tmp2, ".");
            tmp2.clear();
            tmp2.append(tmp.at(0));
            tmp2.append(tmp.at(1));
            w.push_back(atoi(tmp2.c_str()));
        }
        else
            w.push_back(1);
         _accept_charset.push_back(trim(line.at(0)));
    }
    for (unsigned long i = 0; i < _accept_charset.size() - 1; i++)
    {
        if (w.at(i) > w.at(i + 1))
        {
            w.push_back(w.at(i));
            w.erase(w.begin() + i);
            _accept_charset.push_back(_accept_charset.at(i));
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
    if (line.size() == 0)
        return;
    _raw_accept_language = line.at(1);
    line = strsplit(line.at(1), ",");
    for (std::vector<std::string>::iterator it = line.begin(); it != line.end(); it++)
    {
        tmp = strsplit(*it, ";");
        if (tmp.size() > 2)
            return ((void)_accept_language.clear());
        if (tmp.size() == 2 && weight.match(tmp.at(1)).first)
        {
            tmp2 = num.match(weight.match(tmp.at(1)).second).second;
            tmp = strsplit(tmp2, ".");
            tmp2.clear();
            tmp2.append(tmp.at(0));
            tmp2.append(tmp.at(1));
            w.push_back(atoi(tmp2.c_str()));
        }
        else
            w.push_back(1);
         _accept_language.push_back(trim(line.at(0)));
    }
    for (unsigned long i = 0; i < _accept_language.size() - 1; i++)
    {
        if (w.at(i) > w.at(i + 1))
        {
            w.push_back(w[i]);
            w.erase(w.begin() + i);
            _accept_language.push_back(_accept_language.at(i));
            _accept_language.erase(_accept_language.begin() + i);
            i = 0;
        }
    }
}

void RequestParser::allow_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;

    line = header_finder(head, "Allow");
    if (line.size() == 0)
        return;
    line = strsplit(line.at(1), ",");
     for (std::vector<std::string>::iterator it = line.begin(); it != line.end(); it++)
     {
         for (unsigned long i = 0; i < _req_methods.size(); i++)
         {
             if (trim(*it) == _req_methods.at(i))
                break;
            if (i == _req_methods.size() - 1)
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
    if (line.size() == 0)
        return;
    line = strsplit(line.at(1), " ");
    if (line.size() != 2)
        return;
    if (trim(line.at(0)) == "Basic")
        _authorization = line.at(1);
    else
        return;
}

void RequestParser::content_language_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    Regex language("\\A[0-9A-Za-z-]*$");

    line = header_finder(head, "Content-language");
    if (line.size() == 0)
        return;
    _raw_content_language = line.at(1);
    line = strsplit(line.at(1), ",");
    for (std::vector<std::string>::iterator it = line.begin(); it != line.end(); it++)
    {
        if (language.match(trim(*it)).first)
            _content_language.push_back(trim(*it));
    }
}

void RequestParser::content_length_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    
    line = header_finder(head, "Content-Length");
    if (line.size() == 0)
        return;
    if (line.size() == 2)
    {
		try {
			_content_length = std::stoull(line.at(1));
		} catch (const std::exception& e) {
			reportError(400);
			throw std::exception();
		}
    }
}

void RequestParser::content_location_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    
    line = header_finder(head, "Content-Location");
    if (line.size() == 0)
        return;
    if (line.size() == 2)
        _content_location = line.at(1);
}

void RequestParser::content_type_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    std::vector<std::string> tmp;
    
    line = header_finder(head, "Content-Type");
    if (line.size() == 0)
        return;
    _raw_content_type = line.at(1);
    line = strsplit(line.at(1), ";");
    tmp = strsplit(line.at(0), "/");
    if (tmp.size() != 2)
        return;
    _content_type.media_type = tmp.at(0);
    _content_type.subtype = tmp.at(1);
    for (unsigned int i = 1; i < line.size(); i++)
    {
        tmp = strsplit(line.at(i), "=");
        if (tmp.size() != 2)
            return; //error ?
        if (tmp.at(0) == " charset")
            _content_type.charset = tmp.at(1);
        else if (tmp.at(0) == " boundary")
            _content_type.boundary = tmp.at(1);
        else
            return; //error ?
    }
}

void RequestParser::date_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    
    line = header_finder(head, "Date");
    if (line.size() == 0)
        return;
    if (line.size() == 2 && is_http_date(trim(line.at(1))))
        _date = trim(line.at(1));
    else
    {
        reportError(400);
		throw std::exception();
    }
}

void RequestParser::host_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;

    int j = 0;
    for (std::vector<std::string>::iterator it = head.begin(); it != head.end(); it++)
    {
        if (strsplit(*it, ":").at(0) == "Host")
            j++;
    }
    if (j != 1)
    {
        reportError(400);
		throw std::exception();
    }
    line = header_finder(head, "Host");
    if (line.size() == 0)
    {
        reportError(400);
		throw std::exception();
    }
    if (line.size() == 2)
    {
        if (strsplit(line.at(1), ":").size() > 2)
        {
            reportError(400);
		    throw std::exception();
        }
        if (strsplit(line.at(1), ":").size() == 2)
        {
            line = strsplit(line.at(1), ":");
            if (!is_number(line.at(1)))
            {
                reportError(400);
		        throw std::exception();
            }
            _host_name = line.at(0);
            _host_ip = (unsigned short)atoi(line.at(1).c_str());
        }
        else
            _host_name = line.at(1);
    }
}

void RequestParser::referer_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    
    line = header_finder(head, "Referer");
    if (line.size() == 0)
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
            reportError(400);
		    throw std::exception();
        }
    }
}

void RequestParser::transfer_encoding(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    std::vector<std::string> tmp;
    
    line = header_finder(head, "Transfer-Encoding");
    if (line.size() == 0)
        return;
    if (line.at(1) != "chunked")
    {
        reportError(400);
		throw std::exception();
    }
    else
        _transfer_encoding = line.at(1);
}

void RequestParser::user_agent_parser(std::vector<std::string> &head)
{
    std::vector<std::string> line;
    std::vector<std::string> tmp;
    
    line = header_finder(head, "User-Agent");
    if (line.size() == 0)
        return;
    _raw_user_agent = line.at(1);
    tmp = strsplit(line.at(1), " ");
    tmp = strsplit(trim(tmp.at(0)), "/");
    if (tmp.size() != 2)
        return;
    _user_agent.product = tmp.at(0);
    _user_agent.version = tmp.at(1);
    tmp = strsplit(line.at(1), " ");
    for (std::vector<std::string>::iterator it = tmp.begin() + 1; it != tmp.end(); it++)
    {
        _user_agent.comment.append(*it);
        if (it != tmp.end() - 1)
            _user_agent.comment.push_back(' ');
    }
}

void RequestParser::custom_headers(std::vector<std::string> &head)
{
    std::vector<std::string> tmp;
    Regex reg("^([Xx]-[0-9A-Za-z-]+): (.+)$");

    // for (std::vector<std::string>::iterator it = head.begin() + 1; it != head.end(); it++)
    // {
    //     tmp = get_header_name(*it, ':');
    //     for (std::vector<std::string>::iterator h = _headers.begin(); h != _headers.end(); h++)
    //     {
    //         if (tmp.at(0) == *h)
    //             break ;
    //         if (h + 1 == _headers.end())
    //         {
    //             if (reg.match(*it).first)
    //                 _custom_headers.insert(std::pair<std::string,std::string>(tmp.at(0), tmp.at(1)));
    //             else
    //             {
    //                 reportError(400);
	// 	            throw std::exception();
    //             }
    //         }
    //     }
    // }
	for (auto& keyval : head)
	{
		auto res = reg.matchAll(keyval);
		if (res.first)
		{
			auto& match_group = res.second; 
			_custom_headers[match_group.at(1)] = match_group.at(2);
		}
	}

}