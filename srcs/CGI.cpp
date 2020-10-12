#include <CGI.hpp>
#include <RequestRouter.hpp>

std::string envCGItoStr(EnvCGI env)
{
	using E = EnvCGI;
	switch (env)
	{
		case E::AUTH_TYPE: return "AUTH_TYPE";
		case E::CONTENT_LENGTH: return "CONTENT_LENGTH";
		case E::CONTENT_TYPE: return "CONTENT_TYPE";
		case E::GATEWAY_INTERFACE: return "GATEWAY_INTERFACE";
		case E::PATH_INFO: return "PATH_INFO";
		case E::PATH_TRANSLATED: return "PATH_TRANSLATED";
		case E::QUERY_STRING: return "QUERY_STRING";
		case E::REMOTE_ADDR: return "REMOTE_ADDR";
		case E::REMOTE_IDENT: return "REMOTE_IDENT";
		case E::REMOTE_USER: return "REMOTE_USER";
		case E::REQUEST_METHOD: return "REQUEST_METHOD";
		case E::REQUEST_URI: return "REQUEST_URI";
		case E::SCRIPT_NAME: return "SCRIPT_NAME";
		case E::SCRIPT_FILENAME: return "SCRIPT_FILENAME";
		case E::SERVER_NAME: return "SERVER_NAME";
		case E::SERVER_PORT: return "SERVER_PORT";
		case E::SERVER_PROTOCOL: return "SERVER_PROTOCOL";
		case E::SERVER_SOFTWARE: return "SERVER_SOFTWARE";
		case E::null: return "(null)";
	}
}

EnvCGI strToEnvCGI(const std::string& str)
{
	using E = EnvCGI;
	static const std::map<std::string, EnvCGI> map
	({
		{"AUTH_TYPE", E::AUTH_TYPE},
		{"CONTENT_LENGTH", E::CONTENT_LENGTH},
		{"CONTENT_TYPE", E::CONTENT_TYPE},
		{"GATEWAY_INTERFACE", E::GATEWAY_INTERFACE},
		{"PATH_INFO", E::PATH_INFO},
		{"PATH_TRANSLATED", E::PATH_TRANSLATED},
		{"QUERY_STRING", E::QUERY_STRING},
		{"REMOTE_ADDR", E::REMOTE_ADDR},
		{"REMOTE_USER", E::REMOTE_USER},
		{"REMOTE_IDENT", E::REMOTE_IDENT},
		{"REQUEST_METHOD", E::REQUEST_METHOD},
		{"REQUEST_URI", E::REQUEST_URI},
		{"SCRIPT_NAME", E::SCRIPT_NAME},
		{"SCRIPT_FILENAME", E::SCRIPT_FILENAME},
		{"SERVER_NAME", E::SERVER_NAME},
		{"SERVER_PORT", E::SERVER_PORT},
		{"SERVER_PROTOCOL", E::SERVER_PROTOCOL},
		{"SERVER_SOFTWARE", E::SERVER_SOFTWARE},
	});

	if (map.count(str))
		return map.at(str);

	return E::null;	
}

CGI::CGI(RequestParser& request_parser, const std::map<EnvCGI, std::string>& env, std::vector<std::string> command)
	: request(request_parser), env(env), command(command)
{

}

std::vector<std::string> CGI::buildEnv(const std::map<EnvCGI, std::string>& env)
{
	std::vector<std::string> ret;
	ret.reserve(64);
	if (environ)
	{
		size_t i = 0;
		while (environ[i])
		{
			std::string pair(environ[i]);
			std::string key = pair.substr(0, pair.find("="));
			if (strToEnvCGI(key) == EnvCGI::null)
				ret.push_back(environ[i]);
			++i;
		}
	}

	auto& xheaders = request.getCustomHeaders();
	for (auto& pair : xheaders)
		ret.push_back(format_env_key(pair.first) + "=" + pair.second);
	for (auto& var : env)
		ret.push_back(envCGItoStr(var.first) + "=" + var.second);
	ret.push_back("REDIRECT_STATUS=200");
	
	return ret;
}

std::vector<char*> CGI::toArrayOfCStr(const std::vector<std::string>& vec)
{
	std::vector<char*> vec_ptr;
	vec_ptr.reserve(vec.size() + 1);
	for (auto& s : vec)
		vec_ptr.push_back(const_cast<char*>(s.c_str()));
	vec_ptr.push_back(nullptr);
	return vec_ptr;
}

std::vector<std::string> CGI::parseShebangCommand(const std::string& cgi_scriptname)
{
	std::ifstream in(cgi_scriptname);
	if (in.is_open())
	{
		std::string shebang;
		while (std::getline(in, shebang) && shebang.empty());
		if (shebang.find("#!") != 0)
			return {};
		else
		{
			shebang = shebang.substr(2);
			return strsplit(shebang, " ");
		}
	}
	return {};
}


void CGI::scriptError(const std::string& errlog)
{
	Logger& log = Logger::getInstance();
	log.hl(BOLDRED "CGI Failure", std::string(BOLDWHITE) + env.at(EnvCGI::SCRIPT_FILENAME) + ": " + errlog);
	throw ErrorCode(500, "Internal Server Error");
}


void CGI::parseCGIHeader(const std::string& header, CGIResponseHeaders& headers)
{
	Regex rgx("^([^\\s]+):\\s*([^\\s]+.*)$");
	auto res = rgx.matchAll(header);
	if (!res.first)
		scriptError("bad headers in output of script");
	std::string key = res.second.at(1);
	std::string value = res.second.at(2);

	if (key == "Content-Type" || key == "Content-type")
		headers.content_type = value;
	else if (key == "Status")
		headers.status = value;
	else if (key == "Location")
		headers.location = value;
	
}

void CGI::parseCGIResponse(const std::vector<std::string>& vec_headers, FileRequest& file_req)
{
	CGIResponseHeaders headers;
	std::cout << "CGI headers: ";
	for (auto& s : vec_headers)
	{
		std::cout << s << std::endl;
		parseCGIHeader(s, headers);
	}
	
	if (!headers.location.empty())
	{
		file_req.redirect_uri = headers.location;
		file_req.http_code = 302;
		return;
	}

	if (headers.content_type.empty())
		scriptError("missing `Content-Type` header in output of script");

	file_req.content_type = headers.content_type;

	if (!headers.status.empty())
	{
		Regex status_rgx("^(200|302|400|500|501) (?:.+)$");
		auto res = status_rgx.matchAll(headers.status);
		if (!res.first)
			scriptError("bad `Status` header in output of script");
		
		file_req.http_code = std::stoi(res.second.at(1));
		if (file_req.http_code == 400)
			throw ErrorCode(502, "Bad Gateway");
		if (file_req.http_code == 501)
			throw ErrorCode(501, "Not Implemented");
	}
	else
		file_req.http_code = 200;
}

void CGI::executeCGI(FileRequest& file_req)
{
	if (command.at(0) == "auto")
		command = parseShebangCommand(env.at(EnvCGI::SCRIPT_FILENAME));
	if (command.empty())
		scriptError("`execute_cgi` set to auto but missing shebang");

	command.push_back(env.at(EnvCGI::SCRIPT_FILENAME));
	std::vector<std::string> command_env = buildEnv(env);

	for (auto& s : command_env)
		std::cout << s << std::endl;
	// for (auto& s : command)
	// 	std::cout << s << std::endl;

	std::vector<char*> command_c = CGI::toArrayOfCStr(command);
	std::vector<char*> command_env_c = CGI::toArrayOfCStr(command_env);

	file_req.response_buffer.set(new ResponseBufferProcessStream (
		env.at(EnvCGI::SCRIPT_FILENAME), request.getPayload(),
		command[0], command_c, command_env_c, true
	));

	auto headers = file_req.response_buffer->getHeaders();

	if (request.getMethod() == "HEAD")
		file_req.response_buffer.set(new ResponseBuffer());

	parseCGIResponse(headers, file_req);
}