#include <URL.hpp>


// Tokenizer

Tokenizer::Tokenizer(const std::list<std::string>& delim_set, const std::list<std::string>& skip_set)
	: head(0), delim_set(delim_set), skip_set(skip_set) {}

void Tokenizer::reset()
{
	head = 0;
}

// finds in str the first occurence of any of the strings in strs
// returns a pair containing the start index of the found string + its index
// if no match was found, returns std::string::npos for pos
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


// this function retrieves tokens inside str according to the rules
// given during construction.
// - delim_set: this set of strings defines the order of appearance of delimiters.
//				get_token() should be called delim_set.size() + 1 times to retrieve
//				all the tokens.
// - skip_set:	this set MUST be a subset of delim_set. it informs the object which of
//				the delimiters should be excluded from the tokens.
// every call walks the delim_set, returns the next token in str and advances the string.
// the next token spans from the beginning of str to the first character of the first string 
// found amongst the string delimiters in delim_set. usually the front of the delim_set is
// the expected delimiter and popped at the end of the call, but if one of the following one
// is found, next calls will pop front and return empty string until that delimiter is at
// the front to reflect the empty tokens.
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

URL::URL(const URL& o)
{
	*this = o;
}

URL& URL::operator=(const URL& o)
{
	_encoded_url = o._encoded_url;
	_scheme = o._scheme;
	_host = o._host;
	_hosttype = o._hosttype;
	_port = o._port;
	_path = o._path;
	_query = o._query;
	_fragment = o._fragment;
	
	return *this;
}

URL::URL (
	const std::string& scheme,
	const std::string& host,
	const std::string& port,
	const std::string& path,
	const std::string& query,
	const std::string& fragment
)
{
	using C = URL::Component;
	std::string complete_url;

	if (!scheme.empty())
		complete_url += scheme + "://";
	complete_url += URL::encode(C::Host, host);
	if (!port.empty())
		complete_url += ":" + port;
	complete_url += URL::encode(C::Path, path);
	if (!query.empty())
		complete_url += "?" + URL::encode(C::Query, query);
	if (!fragment.empty())
		complete_url += "#" + URL::encode(C::Fragment, fragment);

	*this = URL(complete_url);
}

URL::URL(const std::string& encoded_url)
	: _encoded_url(encoded_url)
{

	// scheme processed separately, since the tokenizer
	// object assumes the presence of a token implies the presence
	// of the previous delimiter. (having "://" isn't required for a host token)
	size_t scheme_delim = _encoded_url.find("://");
	if (scheme_delim != std::string::npos)
	{
		_scheme = _encoded_url.substr(0, scheme_delim);
		_encoded_url = _encoded_url.substr(scheme_delim + 3);
	}

	Tokenizer tk({":", "/", "?", "#"}, {":", "?", "#"});
	_host = tk.get_token(_encoded_url);
	_port = tk.get_token(_encoded_url);
	_path = tk.get_token(_encoded_url);
	_query = tk.get_token(_encoded_url);
	_fragment = _encoded_url;

	_encoded_url = encoded_url;

	validateAllComponents();
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

std::string URL::getFullURL()
{
	return _encoded_url;
}

// Percent encodes the given string from UTF-8. Percent encoding is only applied on non-ASCII characters
// and the characters matching the regex
std::string URL::encode(const Regex& rgx, const std::string& str)
{
	std::stringstream ss;

	for (auto& c : str)
	{
		bool found = rgx.match(std::string(1, c)).first;
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

// Percent encodes the given string from UTF-8. Percent encoding is only applied on non-ASCII characters.
std::string URL::encode(const std::string& str)
{
	std::stringstream ss;

	for (auto& c : str)
	{
		if (c & 0b10000000)
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

std::string URL::encode(URL::Component comp_charset, const std::string& str)
{
	using C = URL::Component;
	switch (comp_charset)
	{
		case C::Scheme: {
			return str;
		}
		case C::Host: {
			return encode(Regex("^[^a-zA-Z\\d-._~!$&'()*+,;=]$"), str);
		}
		case C::Port: {
			return str;
		}
		case C::Path: {
			// added forward slash, that way no need to split into segments to percent encode
			// i'm assuming forward slash cannot be used whatsoever in file names
			return encode(Regex("^[^a-zA-Z\\d-._~!$&'()*+,;=:@/]$"), str);
		}
		case C::Query: {
			return encode(Regex("^[^a-zA-Z\\d-._~!$&'()*+,;=:@/?]$"), str);
		}
		case C::Fragment: {
			return encode(Regex("^[^a-zA-Z\\d-._~!$&'()*+,;=:@/?]$"), str);

		}
	}
}

// Performs percent-encoded to UTF-8 conversion and returns the result
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


void				URL::validateAllComponents()
{
	using C = URL::Component;
	validate(C::Scheme);
	validate(C::Host);
	validate(C::Port);
	validate(C::Path);
	validate(C::Query);
	validate(C::Fragment);
}

// Verifies the syntaxic validity of a specific URL component.
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

// bool	URL::operator==(const URL& o)
// {
// 	auto d = URL::decode;
// 	return	d(_scheme) == d(o._scheme)
// 			&& d()
// }

void	URL::printComponents()
{
	std::cout << "Scheme: " << _scheme << std::endl;
	std::cout << "Host: " << _host << " (" << _hosttype << ")" << std::endl;
	std::cout << "Port: " << _port << std::endl;
	std::cout << "Path: " << _path << std::endl;
	std::cout << "Query: " << _query << std::endl;
	std::cout << "Fragment: " << _fragment << std::endl;
}

void	URL::printDecoded()
{
	auto d = URL::decode;
	std::cout << "Scheme: " << d(_scheme) << std::endl;
	std::cout << "Host: " << d(_host) << " (" << _hosttype << ")" << std::endl;
	std::cout << "Port: " << d(_port) << std::endl;
	std::cout << "Path: " << d(_path) << std::endl;
	std::cout << "Query: " << d(_query) << std::endl;
	std::cout << "Fragment: " << d(_fragment) << std::endl;
}
