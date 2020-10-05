#include <RequestRouter.hpp>
#include <ServerSocketPool.hpp>
#include <CGI.hpp>

RequestRouter::RequestRouter()
	: route_binding(nullptr)
{
	
}

RequestRouter::RequestRouter(const Config& conf)
	: main(conf.main), route_binding(conf.main.get()), saved_binding(conf.main.get())
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

void		RequestRouter::saveBinding()
{
	saved_binding = route_binding;
}
void		RequestRouter::loadBinding()
{
	route_binding = saved_binding;
}

bool		RequestRouter::saveMostSpecificLocation(const std::string& request_uri, ConfBlockDirective*& most_specific_prefix_loc)
{
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
	return false;
}

bool	RequestRouter::bindLocation(const std::string& request_uri)
{
	ConfBlockDirective* most_specific_prefix_loc = nullptr;

	if (saveMostSpecificLocation(request_uri, most_specific_prefix_loc))
		return true;
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

void	RequestRouter::fetchErrorPage(FileRequest& file_req, RequestParser& parsed_request, int code, const std::string& msg)
{
	file_req.http_code = code;
	file_req.http_string = msg;
	file_req.content_type = "text/html";
	file_req.response_buffer.set(new ResponseBuffer);
	std::vector<std::string> vals = getBoundRequestDirectiveValues(DirectiveKey::error_page);
	if (vals.empty())
	{
		std::string tmp = make_html_error_page(code, msg);
		// file_req.file_content.append((BYTE*)tmp.data(), tmp.length());
		file_req.content_length = tmp.length();
		if (parsed_request.getMethod() != "HEAD")
			file_req.response_buffer->get().append((BYTE*)tmp.data(), tmp.length());
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
					// file_req.file_content.appendFile(uri);
					file_req.content_length = file_req.response_buffer->get().size();
					if (parsed_request.getMethod() != "HEAD")
						file_req.response_buffer->get().appendFile(uri);
					return;
				} catch (const std::runtime_error& e) {
					std::string errpage = make_html_error_page(code, msg);
					// file_req.file_content.append((BYTE*)errpage.data(), errpage.length());
					file_req.content_length = errpage.length();
					if (parsed_request.getMethod() != "HEAD")
						file_req.response_buffer->get().append((BYTE*)errpage.data(), errpage.length());
					return;
				}
			}
		std::string errpage = make_html_error_page(code, msg);
		// file_req.file_content.append((BYTE*)errpage.data(), errpage.length());
		file_req.content_length = errpage.length();
		if (parsed_request.getMethod() != "HEAD")
			file_req.response_buffer->get().append((BYTE*)errpage.data(), errpage.length());
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
	return get_current_dir() + res_uri;
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
			std::string path = get_current_dir() + root + uri;
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

void	RequestRouter::fetchFile(FileRequest& file_req, RequestParser& parsed_request, const std::string& request_uri)
{
	std::string path = resolveUriToLocalPath(request_uri);
	file_req.response_buffer.set(new ResponseBuffer);

	struct stat buffer;
	if (stat(path.c_str(), &buffer) != 0)
	{
		fetchErrorPage(file_req, parsed_request, 404, "Not Found");
		return;
	}
	else if (!(buffer.st_mode & S_IFREG))
	{
		auto index_paths = getBoundRequestDirectiveValues(DirectiveKey::index);
		for (auto ipath : index_paths)
		{
			ipath = path + "/" + ipath;
			if (stat(ipath.c_str(), &buffer) == 0 && (buffer.st_mode & S_IFREG))
			{
				try {
					// file_req.file_content.appendFile(ipath);
					if (parsed_request.getMethod() != "HEAD")
						file_req.response_buffer->get().appendFile(ipath);
					file_req.content_length = peek_file_size(ipath);
					file_req.file_path = ipath;
					file_req.http_code = 200;
					file_req.http_string = "OK";
					file_req.last_modified = get_gmt_time(buffer.st_mtime);
					file_req.content_type = get_mime_type(ipath);
					return;
				} catch (const std::runtime_error& e) {
					fetchErrorPage(file_req, parsed_request, 404, "Not Found");
					return;
				}
			}
		}
		fetchErrorPage(file_req, parsed_request, 404, "Not Found");
		return;
	}

	try {
		if (parsed_request.getMethod() != "HEAD")
			file_req.response_buffer->get().appendFile(path);
		file_req.content_length = peek_file_size(path);
		file_req.file_path = path;
		file_req.http_code = 200;
		file_req.http_string = "OK";
		file_req.last_modified = get_gmt_time(buffer.st_mtime);
		file_req.content_type = get_mime_type(path);
	} catch (const std::runtime_error& e) {
		fetchErrorPage(file_req, parsed_request, 404, "Not Found");
		return;
	}
}

std::string	RequestRouter::getAuthUser(const std::string& basic_auth)
{
	auto userpwds = getBoundRequestDirectiveValues(DirectiveKey::auth_basic_user_file);
	for (auto& userpass : userpwds)
	{
		auto entry = tokenizer(userpass, ':');
		std::string pass = entry.at(1);
		if (pass == basic_auth)
			return entry.at(0);
	}
	return "";
}

bool		RequestRouter::checkMethod(RequestParser& parsed_request, FileRequest& file_req)
{
	auto mthds = getBoundRequestDirectiveValues(DirectiveKey::accept_methods);
	if (mthds.empty())
	{
		file_req.allowed_methods.clear();
		return true;
	}
	file_req.allowed_methods = mthds;
	if (std::find(mthds.begin(), mthds.end(), parsed_request.getMethod()) != mthds.end())
		return true;
	fetchErrorPage(file_req, parsed_request, 405, "Method Not Allowed");
	return false;
}

bool	RequestRouter::checkAuthorization(FileRequest& file_req, RequestParser& parsed_request, const std::string& basic_auth)
{

	auto auth_basic_vals = getBoundRequestDirectiveValues(DirectiveKey::auth_basic);
	if (auth_basic_vals.empty() || auth_basic_vals.at(0) == "off")
		return true;
	file_req.realm = auth_basic_vals.at(0);
	if (basic_auth.empty())
	{
		fetchErrorPage(file_req, parsed_request, 401, "Unauthorized");
		return false;
	}
	if (!getAuthUser(basic_auth).empty())
		return true;
	fetchErrorPage(file_req, parsed_request, 403, "Forbidden");
	return false;
}

std::map<EnvCGI, std::string>	RequestRouter::setCGIEnv (
	FileRequest&		file_req,
	RequestParser&		parsed_request,
	HTTPExchange&		ticket
)
{
	using E = EnvCGI;
	URL url(parsed_request.getResource());
	std::string request_path = URL::decode(URL::reformatPath(url.get(URL::Component::Path)));
	std::map<EnvCGI, std::string> env;
	auto auth_basic_val = getBoundRequestDirectiveValues(DirectiveKey::auth_basic);
	if (!auth_basic_val.empty() && auth_basic_val.at(0) != "off")
		env[E::AUTH_TYPE] = "Basic";
	env[E::CONTENT_LENGTH] = std::to_string(parsed_request.getContentLength());
	env[E::CONTENT_TYPE] = parsed_request.getRawContentType(); // NEED IT FOR POST REQUESTS
	env[E::GATEWAY_INTERFACE] = "CGI/1.1";
	std::string pathsplit = getBoundRequestDirectiveValues(DirectiveKey::cgi_split_path_info).at(0);
	auto res = Regex(pathsplit).matchAll(request_path);
	if (!res.first || res.second.size() != 3)
	{
		fetchErrorPage(file_req, parsed_request, 500, "Internal Server Error");
		return {};
	}
	env[E::SCRIPT_NAME] = res.second.at(1);
	env[E::PATH_INFO] = URL::reformatPath(res.second.at(2));
	env[E::SCRIPT_FILENAME] = resolveUriToLocalPath(env[E::SCRIPT_NAME]);
	// check if the script is a file that exists before sending to execution
	struct stat filecheck;
	if (stat(env.at(E::SCRIPT_FILENAME).c_str(), &filecheck) != 0 || !(filecheck.st_mode & S_IFREG))
	{
		fetchErrorPage(file_req, parsed_request, 404, "Not Found");
		return {};
	}
	saveBinding();
	bindServer(parsed_request.getHost(), ticket.listeningAddress(), ticket.listeningPort());
	bool located = bindLocation(env[E::PATH_INFO]);
	if (!located)
		env[E::PATH_INFO].clear();
	env[E::PATH_TRANSLATED] = resolveUriToLocalPath(env[E::PATH_INFO]);
	loadBinding();
	env[E::QUERY_STRING] = url.get(URL::Component::Query);
	env[E::REMOTE_ADDR] = ticket.clientAddress();
	env[E::REMOTE_IDENT] = "";
	env[E::REMOTE_USER] = getAuthUser(parsed_request.getAuthorization());
	env[E::REQUEST_METHOD] = parsed_request.getMethod();
	env[E::REQUEST_URI]	= parsed_request.getResource();
	env[E::SERVER_NAME] = parsed_request.getHost();
	env[E::SERVER_PORT] = std::to_string(ticket.listeningPort());
	env[E::SERVER_PROTOCOL] = "HTTP/1.1";	
	env[E::SERVER_SOFTWARE] = "Webserv/1.0 (Unix)";

	return env;
}

void		RequestRouter::executeCGI(
	FileRequest&		file_req,
	RequestParser&		parsed_request,
	HTTPExchange&		ticket
)
{
	using E = EnvCGI;

	auto env = setCGIEnv(file_req, parsed_request, ticket);
	if (env.empty())
		return;

	auto cgi_bin = getBoundRequestDirectiveValues(DirectiveKey::execute_cgi);
	auto auth_basic_val = getBoundRequestDirectiveValues(DirectiveKey::auth_basic);

	// backup current directory
	std::string cwd_backup = get_current_dir();

	// switch to the script's directory
	size_t sn = env.at(EnvCGI::SCRIPT_FILENAME).rfind(env.at(EnvCGI::SCRIPT_NAME));
	std::string script_dir = env.at(EnvCGI::SCRIPT_FILENAME).substr(0, sn);
	chdir(script_dir.c_str());

	// run script and reload the saved directory
	CGI cgi(parsed_request, env, cgi_bin);
	// cgi.executeCGI(file_req);
	try { cgi.executeCGI(file_req); }
	catch (const ErrorCode& e)
	{
		chdir(cwd_backup.c_str());
		fetchErrorPage(file_req, parsed_request, e.code(), e.str());
		return;
	}
	chdir(cwd_backup.c_str());

	// filling out the remaining request information
	file_req.file_path = env.at(E::SCRIPT_FILENAME);
	struct stat filecheck;
	stat(env.at(E::SCRIPT_FILENAME).c_str(), &filecheck);
	file_req.last_modified = get_gmt_time(filecheck.st_mtime);
	if (!auth_basic_val.empty() && auth_basic_val.at(0) != "off")
		file_req.realm = auth_basic_val.at(0);
}

void	RequestRouter::checkRedirect(RequestParser& parsed_request, HTTPExchange& ticket, FileRequest& file_req)
{
	if (file_req.http_code == 302)
	{
		try {
			URL url(file_req.redirect_uri);
			if (url.isPartialURI())
			{
				parsed_request.getResource() = file_req.redirect_uri;
				file_req = requestFile(parsed_request, ticket);
			}
		} catch (const std::exception& e) {
			file_req = FileRequest();
			fetchErrorPage(file_req, parsed_request, 502, "Bad Gateway");
		}
	}
}

FileRequest	RequestRouter::requestFile (
	RequestParser&		parsed_request,
	HTTPExchange&		ticket
)
{
	URL url(parsed_request.getResource());
	std::string request_path = URL::decode(URL::reformatPath(url.get(URL::Component::Path)));

	std::string request_ip = ticket.listeningAddress();
	unsigned short request_port = ticket.listeningPort();

	FileRequest file_req;
	bindServer(parsed_request.getHost(), request_ip, request_port);
	bool located = bindLocation(request_path);
	if (!located)
		fetchErrorPage(file_req, parsed_request, 404, "Not Found");
	else
	{
		if (checkAuthorization(file_req, parsed_request, parsed_request.getAuthorization())
			&& checkMethod(parsed_request, file_req))
		{
			auto cgi_dir = getBoundRequestDirectiveValues(DirectiveKey::execute_cgi);
			if (!cgi_dir.empty())
				executeCGI(file_req, parsed_request, ticket);
			else
				fetchFile(file_req, parsed_request, request_path);
			checkRedirect(parsed_request, ticket, file_req);
		}
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