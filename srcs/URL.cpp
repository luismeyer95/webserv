#include <URL.hpp>


// Tokenizer

Tokenizer::Tokenizer(const std::list<std::string>& delim_set, const std::list<std::string>& skip_set)
	: head(0), delim_set(delim_set), skip_set(skip_set) {}

void Tokenizer::reset()
{
	head = 0;
}

std::pair<size_t, size_t>	Tokenizer::find_first_of_str
	(const std::string& str, const std::list<std::string>& strs)
{
	size_t pos = std::string::npos;
	size_t len = std::string::npos;

	size_t tmp;
	for (auto& s : strs)
	{
		tmp = str.find(s);
		if (tmp < pos)
		{
			pos = tmp;
			len = s.size();
		}
	}
	
	return {pos, len};
}

std::string Tokenizer::get_token(std::string& str)
{
	if (delim_set.empty())
		return "";
	if (head)
	{
		--head;
		if (skip_set.front() == delim_set.front())
			skip_set.pop_front();
		delim_set.pop_front();
		return "";
	}

	std::string ret;
	if (!str.empty())
	{
		auto p = find_first_of_str(str, delim_set);
		size_t pos = p.first;
		size_t len = p.second;
		std::string delim;
		if (pos != std::string::npos)
			delim = str.substr(pos, len);

		ret = str.substr(0, pos);
		if (pos != std::string::npos)
		{
			auto it = std::find(delim_set.begin(), delim_set.end(), delim);
			head = std::distance(delim_set.begin(), it);
		}

		auto it = std::find(skip_set.begin(), skip_set.end(), delim);

		if (pos != std::string::npos && it != skip_set.end())
			str = str.substr(pos + len);
		else if (pos != std::string::npos)
			str = str.substr(pos);
		else
			str.clear();
	}

	if (skip_set.front() == delim_set.front())
		skip_set.pop_front();
	delim_set.pop_front();

	return ret;
}

// URL

URL::URL() {}

URL::URL(const std::string& encoded_url)
	: _encoded_url(encoded_url)
{
	Tokenizer tk({"://", ":", "/", "?", "#"}, {"://", ":", "?", "#"});

	_scheme = tk.get_token(_encoded_url);
	_host = tk.get_token(_encoded_url);
	_port = tk.get_token(_encoded_url);
	_path = tk.get_token(_encoded_url);
	_query = tk.get_token(_encoded_url);
	_fragment = _encoded_url;

	_encoded_url = encoded_url;

	using C = URL::Component;
	validate(C::Scheme);
	validate(C::Host);
	validate(C::Port);
	validate(C::Path);
	validate(C::Query);
	validate(C::Fragment);
}

std::string& URL::get(URL::Component comp)
{
	using C = URL::Component;
	switch (comp)
	{
		case C::Scheme : return _scheme;
		case C::Host : return _host;
		case C::Port : return _port;
		case C::Path : return _path;
		case C::Query : return _query;
		case C::Fragment : return _fragment;
	}
}

std::string URL::encode(const std::vector<char>& reserved_set, const std::string& str)
{
	std::stringstream ss;

	for (auto& c : str)
	{
		bool found = std::find ( 
			reserved_set.begin(), reserved_set.end(), c
		) != reserved_set.end();
		if (c & 0b10000000 || found)
		{
			unsigned char uc = c;
			ss << "%" << std::hex << static_cast<int>(uc);
			ss << std::dec;
		}
		else
			ss << c;
	}
	return ss.str();
}

std::string URL::decode(const std::string& str)
{
	std::stringstream ss;

	size_t i = 0;
	while (str[i])
	{
		if (str[i] == '%')
		{
			int nb = std::stoi(str.substr(i + 1, 2), nullptr, 16);
			char c = static_cast<char>(nb);
			ss << c;
			i += 3;
		}
		else
			ss << str[i++];
	}
	return ss.str();
}

void URL::validate(URL::Component comp)
{
	using C = URL::Component;
	switch (comp)
	{
		case C::Scheme:
		{
			check (
				"^([a-zA-Z]([a-zA-Z]|\\d|\\+|-|\\.)*)?$", _scheme,
				true, "invalid scheme"
			);
		}
		case C::Host:
		{
			// try to match ipv4 first
			auto ipv4 = Regex (
				"^(([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])\\.){3}([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])$"
			).match(_host);
			if (!ipv4.first)
			{
				auto regname = Regex (
					"^([a-zA-Z\\d-._~!$&'()*+,;=]|%[a-fA-F\\d]{2})*$"
				).match(_host);

				if (!regname.first)
					throw std::runtime_error("invalid authority component");
				else
					_hosttype = "regname";
			}
			else
				_hosttype = "ipv4";
		}
		case C::Port:
		{
			auto portnb = Regex (
				"^\\d*$"
			).match(_port);
			auto err = std::runtime_error("invalid port number");
			if (portnb.first && !portnb.second.empty())
			{
				int port;
				try {
					port = std::stoi(_port);
				} catch (const std::out_of_range& e) {
					throw err;
				}
				if (port < 0 || port > 65535)
					throw err;
			}
			else if (!portnb.second.empty())
				throw err;
		}
		case C::Path:
		{
			check (
				"^(\\/([a-zA-Z\\d-._~!$&'()*+,;=:@]|%[a-fA-F\\d]{2})*)*$", _path,
				true, "invalid path component"
			);
		}
		case C::Query:
		{
			check (
				"^(([a-zA-Z\\d-._~!$&'()*+,;=:@/?]|%[a-fA-F\\d]{2})*)*$", _query,
				true, "invalid query component"
			);
		}
		case C::Fragment:
		{
			check (
				"^(([a-zA-Z\\d-._~!$&'()*+,;=:@/?]|%[a-fA-F\\d]{2})*)*$", _fragment,
				true, "invalid fragment component"
			);
		}
	}
}

bool		URL::check(const std::string& rgx, const std::string& str, bool thrw, const std::string& error)
{
	auto res = Regex(rgx).match(str);
	if (!res.first && thrw)
		throw std::runtime_error(error);
	return res.first;
}

// REGEX FOR COMPONENTS

// unreserved : ^[a-zA-Z\d-._~]$
// subdelims : ^[!$&'()*+,;=]$
// gen-delims : ^[:/?#\[\]@]$

// scheme : ^[a-zA-Z]([a-zA-Z]|\d|\+|-|\.)*$
// "://"
// authority
	// host : 
		// ipv4 : ^(([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])\.){3}([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])$
			// 0 ... 255 : ^([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])$
			// a.a.a.a	 : ^(a\.){3}a$
		// reg-name : ^([a-zA-Z\d-._~!$&'()*+,;=]|%[a-fA-F\d]{2})*$
	// port ^\d*$
// path : 
	// segment : ^([a-zA-Z\d-._~!$&'()*+,;=:@]|%[a-fA-F\d]{2})*$
	// segment-nz : ^([a-zA-Z\d-._~!$&'()*+,;=:@]|%[a-fA-F\d]{2})+$
	// segment-nz-nc : ^([a-zA-Z\d-._~!$&'()*+,;=@]|%[a-fA-F\d]{2})+$

	// path-abempty : ^(\/([a-zA-Z\d-._~!$&'()*+,;=:@]|%[a-fA-F\d]{2})*)*$
// query = fragment : ^(([a-zA-Z\d-._~!$&'()*+,;=:@/?]|%[a-fA-F\d]{2})*)*$
	


void URL::printComponents()
{
	std::cout << "Scheme: " << _scheme << std::endl;
	std::cout << "Host: " << _host << " (" << _hosttype << ")" << std::endl;
	std::cout << "Port: " << _port << std::endl;
	std::cout << "Path: " << _path << std::endl;
	std::cout << "Query: " << _query << std::endl;
	std::cout << "Fragment: " << _fragment << std::endl;
}

void URL::printDecoded()
{
	auto f = URL::decode;
	std::cout << "Scheme: " << f(_scheme) << std::endl;
	std::cout << "Host: " << f(_host) << " (" << _hosttype << ")" << std::endl;
	std::cout << "Port: " << f(_port) << std::endl;
	std::cout << "Path: " << f(_path) << std::endl;
	std::cout << "Query: " << f(_query) << std::endl;
	std::cout << "Fragment: " << f(_fragment) << std::endl;
}
