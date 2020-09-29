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
		static std::vector<std::string>		parseShebangCommand(const std::string& cgi_scriptname);
		static std::vector<char*>			toArrayOfCStr(const std::vector<std::string>& vec);
		static std::vector<std::string>		buildEnv(const std::map<EnvCGI, std::string>& env);


		static void			parseCGIResponse(const std::string& response, FileRequest& file_req);
		static void			parseCGIHeader(const std::string& header, CGIResponseHeaders& headers);

		static void			redirectLocalURI(const std::string& location_value);



		static std::string	runProcess(const char *bin, char **cmd, char **env);
		static void 		readProcessOutput (int (&pip)[2], pid_t timer_pid, std::string& out);

		static void			scriptError(const std::string& errlog);
		
	public:
		static void			executeCGI (
			FileRequest&		file_req,
			const std::map<EnvCGI, std::string>& env,
			std::vector<std::string> command
		);

};