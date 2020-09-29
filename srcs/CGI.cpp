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
			// std::cout << "KEY=" << key << ", " << (strToEnvCGI(key) != EnvCGI::null) << std::endl;
			if (strToEnvCGI(key) == EnvCGI::null)
				ret.push_back(environ[i]);
			++i;
		}
	}
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
	log.hl(BOLDRED "CGI Failure", std::string(BOLDWHITE) + errlog);
	throw ErrorCode(500, "Internal Server Error");
}

void CGI::readProcessOutput (
	int (&pip)[2], pid_t timer_pid,
	std::string& out
)
{
	std::string line;
	int in_bkp = dup(0);
	int status;

	dup2(pip[0], 0);
	close(pip[0]);
	while (std::getline(std::cin, line, '\n'))
	{
		out += line;
		if (std::cin.good())
			out += "\n";
	}

	waitpid(WAIT_ANY, &status, 0);
	dup2(in_bkp, 0);
	close(in_bkp);
	std::cin.clear();
	if (WIFEXITED(status))
	{
		kill(timer_pid, SIGKILL);
		waitpid(timer_pid, nullptr, 0);
	}
	else
	{
		while (wait(nullptr) != -1);
		throw ErrorCode(500, "Internal Server Error");
	}
}

std::string CGI::runProcess(const char *bin, char **cmd, char **env)
{
	int pip[2];
	std::string out;

	if (pipe(pip) == -1)
		scriptError(std::string("pipe(): ") + strerror(errno));
	pid_t worker_pid = fork();
	if (worker_pid == -1)
		scriptError(std::string("fork(): ") + strerror(errno));
	if (!worker_pid)
	{
		setpgid(0, 0);
		close(pip[0]);
		dup2(pip[1], 1);
		close(pip[1]);
		execve(bin, cmd, env);
		exit(0);
	}
	else if (worker_pid > 0)
	{
		close(pip[1]);
		pid_t timer_pid = fork();
		if (timer_pid == -1)
			scriptError(std::string("fork(): ") + strerror(errno));
		if (!timer_pid)
		{
			close(pip[0]);
			usleep(1000000);
			if (kill(-worker_pid, SIGTERM) == -1)
				kill(-worker_pid, SIGKILL);
			exit(0);
		}
		else if (timer_pid > 0)
			readProcessOutput
				(pip, timer_pid, out);
	}
	return out;
}

void CGI::parseCGIHeader(const std::string& header, CGIResponseHeaders& headers)
{
	Regex rgx("^([^\\s]+):\\s*([^\\s]+.*)$");
	auto res = rgx.matchAll(header);
	if (!res.first)
		throw ErrorCode(500, "Internal Server Error");
	std::string key = res.second.at(1);
	std::string value = res.second.at(2);

	if (key == "Content-Type" || key == "Content-type")
		headers.content_type = value;
	else if (key == "Status")
		headers.status = value;
	else if (key == "Location")
		headers.location = value;
	
}

void splitHeaderBody(const std::string& response, std::vector<std::string>& headers, std::string& body)
{
	size_t header_break = 0;
	size_t break_len = 0;
	size_t double_lf = response.find("\n\n");
	size_t double_crlf = response.find("\r\n\r\n");

	if (double_lf < double_crlf)
	{
		header_break = double_lf;
		break_len = 2;
	}
	else
	{
		header_break = double_crlf;
		break_len = 4;
	}
	
	if (header_break == std::string::npos)
		throw ErrorCode(500, "Internal Server Error");
	std::string str_headers = response.substr(0, header_break);
	headers = strsplit(str_headers, "\n");
	if (headers.empty())
		throw ErrorCode(500, "Internal Server Error");
	body = response.substr(header_break + break_len);

}

void CGI::parseCGIResponse(const std::string& response, FileRequest& file_req)
{
	std::vector<std::string> vec_headers;
	std::string body;

	splitHeaderBody(response, vec_headers, body);

	// for (auto& s : vec_headers)
	// 	std::cout << "HEADER=" << s << std::endl;

	CGIResponseHeaders headers;
	for (auto& s : vec_headers)
		parseCGIHeader(s, headers);
	
	if (!headers.location.empty())
	{
		file_req.redirect_uri = headers.location;
		file_req.http_code = 302;
		return;
	}

	if (!body.empty() && headers.content_type.empty())
		throw ErrorCode(500, "Internal Server Error");
	
	file_req.content_type = headers.content_type;

	if (!headers.status.empty())
	{
		Regex status_rgx("^(200|302|400|501) (?:.+)$");
		auto res = status_rgx.matchAll(headers.status);
		if (!res.first)
			throw ErrorCode(500, "Internal Server Error");
		
		file_req.http_code = std::stoi(res.second.at(1));
		if (file_req.http_code == 400)
			throw ErrorCode(400, "Bad Request");
		if (file_req.http_code == 501)
			throw ErrorCode(501, "Not Implemented");
	}
	else
		file_req.http_code = 200;

	file_req.file_content.append((BYTE*)body.data(), body.size());
}

void CGI::executeCGI (
	FileRequest&		file_req,
	const std::map<EnvCGI, std::string>& env,
	std::vector<std::string> command
)
{
	if (command.at(0) == "auto")
		command = parseShebangCommand(env.at(EnvCGI::SCRIPT_FILENAME));
	if (command.empty())
		throw ErrorCode(500, "Internal Server Error");

	command.push_back(env.at(EnvCGI::SCRIPT_FILENAME));
	std::vector<std::string> command_env = buildEnv(env);

	// for (auto& s : command_env)
	// 	std::cout << s << std::endl;
	// for (auto& s : command)
	// 	std::cout << s << std::endl;

	std::vector<char*> command_c = CGI::toArrayOfCStr(command);
	std::vector<char*> command_env_c = CGI::toArrayOfCStr(command_env);

	
	std::string cgi_content = runProcess(command.at(0).c_str(), &command_c[0], &command_env_c[0]);
	// std::cout << "FULL CGI RESPONSE:\n" << cgi_content << "\n____________________\n";

	parseCGIResponse(cgi_content, file_req);
}