#pragma once

#include <header.h>
#include <ErrorCode.hpp>
#include <Logger.hpp>
#include <URL.hpp>

struct FileRequest;
class RequestRouter;

enum class EnvCGI
{
	AUTH_TYPE, CONTENT_LENGTH, CONTENT_TYPE, GATEWAY_INTERFACE,
	PATH_INFO, PATH_TRANSLATED, QUERY_STRING, REMOTE_ADDR,
	REMOTE_IDENT, REMOTE_USER, REQUEST_METHOD, REQUEST_URI,
	SCRIPT_NAME, SCRIPT_FILENAME, SERVER_NAME, SERVER_PORT,
	SERVER_PROTOCOL, SERVER_SOFTWARE, null
};

struct CGIResponseHeaders
{
	std::string	status;
	std::string content_type;
	std::string location;
};

std::string envCGItoStr(EnvCGI env);
EnvCGI		strToEnvCGI(const std::string& str);

class CGI
{
	private:
		const std::map<EnvCGI, std::string>&	env;
		std::vector<std::string>				command;

		std::vector<std::string>		parseShebangCommand(const std::string& cgi_scriptname);
		std::vector<char*>			toArrayOfCStr(const std::vector<std::string>& vec);
		std::vector<std::string>		buildEnv(const std::map<EnvCGI, std::string>& env);


		void			parseCGIResponse(const std::string& response, FileRequest& file_req);
		void			parseCGIHeader(const std::string& header, CGIResponseHeaders& headers);

		void			redirectLocalURI(const std::string& location_value);

		std::string	runProcess(const char *bin, char **cmd, char **env);
		void 		readProcessOutput (int (&pip)[2], pid_t timer_pid, std::string& out);

		void		splitHeaderBody(const std::string& response, std::vector<std::string>& headers, std::string& body);


		void			scriptError(const std::string& errlog);
		
	public:
		CGI(const std::map<EnvCGI, std::string>& env, std::vector<std::string> command);
		void	executeCGI(FileRequest& file_req);

};