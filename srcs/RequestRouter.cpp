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

bool	RequestRouter::hasMethod(const std::string& method, ConfBlockDirective& location_block)
{
	try
	{
		auto dir = getDirective(location_block, DirectiveKey::accept_methods);
		return std::find(dir.values.begin(), dir.values.end(), method) != dir.values.end();
	}
	catch (const std::exception& e) { return true; }
}

bool	RequestRouter::bindLocation(const std::string& request_uri, const std::string& request_method)
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
				if (res.first && hasMethod(request_method, block))
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
					file_req.content_length = peek_file_size(uri);
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

std::string RequestRouter::expandCaptures(std::string to_expand, const std::vector<std::string>& match_group)
{
	static Regex capture_rgx("^(?:.*[^\\\\])?(\\$\\d+)(?:[^\\d].*)?$");
	auto cap = capture_rgx.matchAll(to_expand);
	while (cap.first)
	{
		if (cap.second.size() <= 1)
			return to_expand;
		std::string capvar = cap.second.at(1);
		size_t cpos = to_expand.find(capvar);
		size_t clen = capvar.size();
		try {
			unsigned long cap_nb = std::stoul(&capvar[1]);
			if (cap_nb < match_group.size())
				to_expand.replace(cpos, clen, match_group.at(cap_nb));
			else
				to_expand.erase(cpos, clen);
		} catch (...) {
			to_expand.erase(cpos, clen);
		}
		cap = capture_rgx.matchAll(to_expand);
	}
	auto end = std::remove_if
		(to_expand.begin(), to_expand.end(), [] (char c) { return c == '\\'; });
	to_expand.resize(std::distance(to_expand.begin(), end));

	return to_expand;
}

std::string RequestRouter::getCurrentLocationPrefix()
{
	if (!route_binding || route_binding->key != ContextKey::location)
		return {};
	if (route_binding->prefixes.at(0) == "~" || route_binding->prefixes.at(0) == "=")
		return route_binding->prefixes.at(1);
	else
		return route_binding->prefixes.at(0);
}

std::string RequestRouter::resolveAliasUri(const std::string& request_uri, ConfBlockDirective& block)
{
	std::string alias = getDirective(block, DirectiveKey::alias).values.at(0);
	std::string loc_regex = getCurrentLocationPrefix();

	auto res = Regex(loc_regex).matchAll(request_uri);
	std::string matched = res.second.at(0);
	size_t pos = request_uri.find(matched);
	size_t len = matched.size();

	std::string expanded_alias = expandCaptures(alias, res.second);
	std::string res_uri = request_uri;
	res_uri.replace(pos, len, expanded_alias);
	return get_current_dir() + res_uri;
}

std::string 	RequestRouter::resolveUriToLocalPath(const std::string& request_uri)
{
	ConfBlockDirective* tmp = route_binding;

	while (tmp)
	{
		try {
			std::string root = getDirective(*tmp, DirectiveKey::root).values.at(0);
			std::string path = URL::reformatPath(get_current_dir() + root + request_uri);
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

bool	RequestRouter::assertOrError(bool expr, FileRequest& file_req, RequestParser& parsed_request, int code, const std::string& msg)
{
	if (!expr)
	{
		fetchErrorPage(file_req, parsed_request, code, msg);
		return false;
	}
	return true;
}

void	RequestRouter::putFile(FileRequest& file_req, RequestParser& parsed_request, const std::string& request_uri)
{
	file_req.response_buffer.set(new ResponseBuffer);
	std::string path = resolveUriToLocalPath(request_uri);

	struct stat buffer;
	int st_ret = stat(path.c_str(), &buffer);
	if (st_ret != 0 || (buffer.st_mode & S_IFREG))
	{
		int in;
		if (!assertOrError((in = open(path.c_str(), O_RDWR | O_CREAT, 0644)) != -1,
			file_req, parsed_request, 500, "Internal Server Error"))
			return;
		const void *buf = parsed_request.getPayload().get();
		ssize_t len = parsed_request.getContentLength();
		if (!assertOrError(write(in, buf, len) == len, file_req, parsed_request, 500, "Internal Server Error"))
		{
			close(in);
			return;
		}
		if (!st_ret && (buffer.st_mode & S_IFREG))
		{
			file_req.http_code = 200;
			file_req.http_string = "OK";
		}
		else
		{
			file_req.http_code = 201;
			file_req.http_string = "Created";
		}
		file_req.content_location = request_uri;
		file_req.content_length = 0;
		close(in);
	}
	else
		fetchErrorPage(file_req, parsed_request, 500, "Internal Server Error");
}

bool	RequestRouter::checkAutoindex(FileRequest& file_req, RequestParser& parsed_request, const std::string& path)
{
	auto autoindex_vals = getBoundRequestDirectiveValues(DirectiveKey::autoindex);

	if (autoindex_vals.empty() || autoindex_vals.at(0) == "off")
		return false;

	std::string autoindex_html = http_index(path);
	if (parsed_request.getMethod() != "HEAD")
		file_req.response_buffer->get().append((BYTE*)autoindex_html.c_str(), autoindex_html.size());
	file_req.content_length = autoindex_html.size();
	file_req.file_path = path;
	file_req.http_code = 200;
	file_req.http_string = "OK";
	file_req.content_type = "text/html";

	return true;
}

void	RequestRouter::fetchFile(FileRequest& file_req, RequestParser& parsed_request, const std::string& request_uri)
{
	file_req.response_buffer.set(new ResponseBuffer);
	std::string path = resolveUriToLocalPath(request_uri);

	Logger& log = Logger::getInstance();
	// log.hl(BOLDWHITE "RESOLVED PATH", path);

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
		if (!checkAutoindex(file_req, parsed_request, path))
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

	// std::cout << "ALLOWED: " << std::endl;
	// for (auto& s : mthds)
	// 	std::cout << s;
	// std::cout << std::endl;

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
	env[E::CONTENT_TYPE] = parsed_request.getRawContentType();
	env[E::GATEWAY_INTERFACE] = "CGI/1.1";

	auto res = Regex(getCurrentLocationPrefix()).matchAll(request_path);
	env[E::SCRIPT_NAME] = expandCaptures(getBoundRequestDirectiveValues(DirectiveKey::cgi_script_name).at(0), res.second);
	env[E::SCRIPT_NAME] = URL::reformatPath(env[E::SCRIPT_NAME]);
	env[E::PATH_INFO] = expandCaptures(getBoundRequestDirectiveValues(DirectiveKey::cgi_path_info).at(0), res.second);
	env[E::PATH_INFO] = URL::reformatPath(env[E::PATH_INFO]);
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
	bool located = bindLocation(env[E::PATH_INFO], parsed_request.getMethod());
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
			fetchErrorPage(file_req, parsed_request, 500, "Internal Server Error");
		}
	}
}

// bool	RequestRouter::checkBodyLength(RequestParser& parsed_request, FileRequest& file_req)
// {
// 	auto max_body_dir = getBoundRequestDirectiveValues(DirectiveKey::max_request_body);
// 	if (max_body_dir.empty())
// 		return true;
// 	if (parsed_request.getContentLength() <= std::stoull(max_body_dir.at(0)))
// 		return true;
// 	fetchErrorPage(file_req, parsed_request, 413, "Payload Too Large");
// 	return false;
// }

bool RequestRouter::methodIsEither(const std::string& method, const std::vector<std::string>& list)
{
	return std::find(list.begin(), list.end(), method) != list.end();
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
	bool located = bindLocation(request_path, parsed_request.getMethod());
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
			else if (methodIsEither(parsed_request.getMethod(), {"GET", "HEAD"}))
				fetchFile(file_req, parsed_request, request_path);
			else if (parsed_request.getMethod() == "PUT")
				putFile(file_req, parsed_request, request_path);
			else
			{
				fetchErrorPage(file_req, parsed_request, 405, "Method Not Allowed");
				return file_req;
			}
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