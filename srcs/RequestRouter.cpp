#include <RequestRouter.hpp>

RequestRouter::RequestRouter()
	: route_binding(nullptr)
{
	
}

RequestRouter::RequestRouter(const Config& conf)
	: main(conf.main), route_binding(conf.main.get())
{

}

RequestRouter& RequestRouter::operator=(const Config& conf)
{
	this->main = conf.main;
	route_binding = conf.main.get();
	return *this;
}


ConfBlockDirective&		RequestRouter::getBlock(ConfBlockDirective& b, ContextKey key)
{
	auto it = std::find_if (
		b.blocks.begin(),
		b.blocks.end(),
		[&] (ConfBlockDirective blk) { return blk.key == key; }
	);
	if (it == b.blocks.end())
		throw std::runtime_error("RequestRouter: could not locate requested element in structure");
	return *it;
}

ConfDirective&			RequestRouter::getDirective(ConfBlockDirective& b, DirectiveKey key)
{
	auto it = std::find_if (
		b.directives.begin(),
		b.directives.end(),
		[&] (ConfDirective dir) { return dir.key == key; }
	);
	if (it == b.directives.end())
		throw std::runtime_error("RequestRouter: could not locate requested element in structure");
	return *it;
}

bool	RequestRouter::bindLocation(const std::string& request_uri)
{
	ConfBlockDirective* most_specific_prefix_loc = nullptr;
	for (auto& block : route_binding->blocks)
	{
		if (block.key == ContextKey::location)
		{
			std::vector<std::string>& prefixes = block.prefixes;
			if (prefixes.at(0) == "=")
			{
				std::string expr = Regex::escapeSymbols(prefixes.at(1));
				auto res = Regex("^" + expr + "$").match(request_uri);
				if (res.first)
				{
					route_binding = &block;
					return true;
				}
			}
			if (prefixes.at(0) != "~")
			{
				std::string expr = Regex::escapeSymbols(prefixes.at(0));
				auto res = Regex("^" + expr).match(request_uri);
				if (res.first && (!most_specific_prefix_loc ||
					prefixes.at(0).size() > most_specific_prefix_loc->prefixes.at(0).size()))
					most_specific_prefix_loc = &block;
			}
		}
	}
	for (auto& block : route_binding->blocks)
		if (block.key == ContextKey::location)
		{
			std::vector<std::string>& prefixes = block.prefixes;
			if (prefixes.at(0) == "~")
			{
				auto res = Regex(prefixes.at(1)).match(request_uri);
				if (res.first)
				{
					route_binding = &block;
					return true;
				}
			}
		}
	if (most_specific_prefix_loc)
	{
		route_binding = most_specific_prefix_loc;
		return true;
	}
	return false;
}

void	RequestRouter::bindServer (
	const std::string& request_servname,
	const std::string& request_ip_host,
	unsigned short request_port
)
{
	ConfBlockDirective* default_server = nullptr;

	for (auto& block : main->blocks)
	{
		if (block.key == ContextKey::server)
		{
			std::string host_port = RequestRouter::getDirective(block, DirectiveKey::listen).values.at(0);
			auto tokens = tokenizer(host_port, ':');
			std::string host = tokens.at(0);
			if (host == "localhost")
				host = "127.0.0.1";
			unsigned short port = std::stoi(tokens.at(1));
			if (request_ip_host == host && request_port == port)
			{
				if (!default_server)
					default_server = &block;
				std::vector<std::string> servnames;
				try {
					servnames = RequestRouter::getDirective(block, DirectiveKey::server_name).values;
					for (auto& name : servnames)
					{
						if (name == request_servname)
						{
							route_binding = &block;
							return;
						}
					}
				} catch (const std ::runtime_error& e) {}				
			}
		}
	}
	if (default_server)
		route_binding = default_server;
}

void	RequestRouter::fetchErrorPage(FileRequest& file_req, int code, const std::string& msg)
{
	file_req.http_code = code;
	file_req.http_string = msg;
	std::vector<std::string> vals = getBoundRequestDirectiveValues(DirectiveKey::error_page);
	if (vals.empty())
	{
		std::string tmp = make_html_error_page(code, msg);
		file_req.file_content.append((BYTE*)tmp.data(), tmp.length());
	}
	else
	{
		for (auto& s : vals)
			if (s == std::to_string(code))
			{
				std::string uri = "." + *vals.rbegin();
				size_t xxx = uri.find("xxx");
				uri.replace(xxx, 3, std::to_string(code));
				try {
					file_req.file_content.appendFile(uri);
					return;
				} catch (const std::runtime_error& e) {
					std::string errpage = make_html_error_page(code, msg);
					file_req.file_content.append((BYTE*)errpage.data(), errpage.length());
					return;
				}
			}
		std::string errpage = make_html_error_page(code, msg);
		file_req.file_content.append((BYTE*)errpage.data(), errpage.length());
	}
}

std::string RequestRouter::resolveAliasUri(const std::string& request_uri, ConfBlockDirective& block)
{
	std::string alias = getDirective(block, DirectiveKey::alias).values.at(0);

	std::string loc_regex;
	if (route_binding->prefixes.at(0) == "~")
		loc_regex = route_binding->prefixes.at(1);
	else
		loc_regex = route_binding->prefixes.at(0);

	auto res = Regex(loc_regex).matchAll(request_uri);
	std::string matched = res.second.at(0);
	size_t pos = request_uri.find(matched);
	size_t len = matched.size();

	if (route_binding->prefixes.at(0) == "~")
	{
		Regex capture_rgx("^(?:.*[^\\\\])?(\\$\\d+)(?:[^\\d].*)?$");
		auto cap = capture_rgx.matchAll(alias);
		while (cap.first)
		{
			std::string capvar = cap.second.at(1);
			size_t cpos = alias.find(capvar);
			size_t clen = capvar.size();
			try {
				unsigned long cap_nb = std::stoul(&capvar[1]);
				if (cap_nb < res.second.size())
					alias.replace(cpos, clen, res.second.at(cap_nb));
				else
					alias.erase(cpos, clen);
			} catch (...) {
				alias.erase(cpos, clen);
			}
			cap = capture_rgx.matchAll(alias);
		}
		auto end = std::remove_if
			(alias.begin(), alias.end(), [] (char c) { return c == '\\'; });
		alias.resize(std::distance(alias.begin(), end));
	}
	std::string res_uri = request_uri;
	res_uri.replace(pos, len, alias);
	return "." + res_uri;
}

std::string 	RequestRouter::resolveUriToLocalPath(const std::string& request_uri)
{
	ConfBlockDirective* tmp = route_binding;

	while (tmp)
	{
		try {
			std::string root = getDirective(*tmp, DirectiveKey::root).values.at(0);
			if (root == "/")
				root = "";
			std::string uri("");
			if (request_uri != "/")
				uri = request_uri;
			std::string path = "." + root + uri;
			return path;
		} catch (...) {}
		try {
			std::string alias = resolveAliasUri(request_uri, *tmp);
			return alias;
		} catch (...) {}
		tmp = tmp->parent;
	}
	// should never reach here
	return "";	
}

void	RequestRouter::fetchFile(FileRequest& file_req, const std::string& request_uri)
{
	std::string path = resolveUriToLocalPath(request_uri);
	std::cout << path << std::endl;

	struct stat buffer;
	if (stat(path.c_str(), &buffer) != 0)
	{
		fetchErrorPage(file_req, 404, "Not Found");
		return;
	}
	else if (!(buffer.st_mode & S_IFREG))
	{
		auto index_paths = getBoundRequestDirectiveValues(DirectiveKey::index);
		for (auto ipath : index_paths)
		{
			ipath = path + "/" + ipath;
			std::cout << ipath << std::endl;
			if (stat(ipath.c_str(), &buffer) == 0 && (buffer.st_mode & S_IFREG))
			{
				try {
					file_req.file_content.appendFile(ipath);
					file_req.file_path = ipath;
					file_req.http_code = 200;
					file_req.http_string = "OK";
					file_req.last_modified = get_gmt_time(buffer.st_mtime);
					return;
				} catch (const std::runtime_error& e) {
					fetchErrorPage(file_req, 404, "Not Found");
					return;
				}
			}
		}
		fetchErrorPage(file_req, 404, "Not Found");
		return;
	}

	try {
		file_req.file_content.appendFile(path);
		file_req.file_path = path;
		file_req.http_code = 200;
		file_req.http_string = "OK";
		file_req.last_modified = get_gmt_time(buffer.st_mtime);
	} catch (const std::runtime_error& e) {
		fetchErrorPage(file_req, 404, "Not Found");
		return;
	}
}

bool	RequestRouter::checkAuthorization(FileRequest& file_req, const std::string& basic_auth)
{
	(void)file_req;
	(void)basic_auth;

	auto auth_basic_vals = getBoundRequestDirectiveValues(DirectiveKey::auth_basic);
	if (auth_basic_vals.empty() || auth_basic_vals.at(0) == "off")
		return true;
	file_req.realm = auth_basic_vals.at(0);
	if (basic_auth.empty())
	{
		fetchErrorPage(file_req, 401, "Unauthorized");
		return false;
	}
	auto userpwds = getBoundRequestDirectiveValues(DirectiveKey::auth_basic_user_file);
	for (auto& userpass : userpwds)
	{
		std::string pass = tokenizer(userpass, ':').at(1);
		if (pass == basic_auth)
			return true;
	}
	fetchErrorPage(file_req, 403, "Forbidden");
	return false;
}

// FileRequest	RequestRouter::requestFile (
// 	const std::string&	request_uri,
// 	const std::string&	request_servname,
// 	const std::string&	request_ip_host,
// 	unsigned short		request_port,
// 	const std::string&	basic_auth
// )
// {
// 	FileRequest file_req;
// 	bindServer(request_servname, request_ip_host, request_port);
// 	bool located = bindLocation(request_uri);
// 	if (!located)
// 		fetchErrorPage(file_req, 404, "Not Found");
// 	else
// 	{
// 		if (checkAuthorization(file_req, basic_auth))
// 			fetchFile(file_req, request_uri);
// 	}
// 	return file_req;
// }

FileRequest	RequestRouter::requestFile (
	RequestParser&		parsed_request,
	const std::string&	request_ip,
	unsigned short		request_port
)
{
	FileRequest file_req;
	bindServer(parsed_request.getHost(), request_ip, request_port);
	bool located = bindLocation(parsed_request.getResource());
	if (!located)
		fetchErrorPage(file_req, 404, "Not Found");
	else
	{
		if (checkAuthorization(file_req, parsed_request.getAuthorization()))
			fetchFile(file_req, parsed_request.getResource());
	}
	return file_req;
}

std::vector<std::string>	RequestRouter::getBoundRequestDirectiveValues(DirectiveKey dirkey)
{
	ConfBlockDirective* tmp = route_binding;
	while (tmp)
	{
		try {
			auto dirvals = getDirective(*tmp, dirkey).values;
			return dirvals;
		} catch (const std::runtime_error& e) {}
		tmp = tmp->parent;
	}
	return {};
}