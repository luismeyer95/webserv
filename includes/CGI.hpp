#pragma once

#include <header.h>

struct FileRequest;

enum class EnvCGI
{
	AUTH_TYPE, CONTENT_LENGTH, CONTENT_TYPE, GATEWAY_INTERFACE,
	PATH_INFO, PATH_TRANSLATED, QUERY_STRING, REMOTE_ADDR,
	REMOTE_IDENT, REMOTE_USER, REQUEST_METHOD, REQUEST_URI,
	SCRIPT_NAME, SCRIPT_FILENAME, SERVER_NAME, SERVER_PORT,
	SERVER_PROTOCOL, SERVER_SOFTWARE, count
};

class CGI
{
	public:
		static void executeCGI (
			FileRequest&		file_req,
			const std::map<EnvCGI, std::string>& env
		);

};