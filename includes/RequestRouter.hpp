#pragma once

#include <Conf/Config.hpp>
#include <ByteBuffer.hpp>

class ServerSocketPool;

struct FileRequest
{
	int					http_code;
	std::string			http_string;

	std::string			file_path;
	std::string			last_modified;
	ByteBuffer			file_content;

	std::string			realm;
};

class RequestRouter
{
	friend class ServerSocketPool;

	private:
		void		bindServer (
			const std::string& request_servname,
			const std::string& request_ip_host,
			unsigned short request_port
		);
		bool		bindLocation(const std::string& request_uri);

		std::shared_ptr<ConfBlockDirective> main;
		ConfBlockDirective*			route_binding;
	public:
		RequestRouter();
		RequestRouter(const Config& conf);
		RequestRouter& operator=(const Config& conf);

		FileRequest	requestFile (
			const std::string&	request_uri,
			const std::string&	request_servname,
			const std::string&	request_ip_host,
			unsigned short		request_port,
			const std::string&	basic_auth = {}
		);

		void fetchErrorPage(FileRequest& file_req, int code, const std::string& msg);
		void fetchFile(FileRequest& file_req, const std::string& request_uri);
		std::string resolveUriToLocalPath(const std::string& request_uri);
		std::string resolveAliasUri(const std::string& request_uri, ConfBlockDirective& block);

		bool		checkAuthorization(FileRequest& file_req, const std::string& basic_auth);


		std::vector<std::string>		getBoundRequestDirectiveValues(DirectiveKey dirkey);

		static ConfBlockDirective&		getBlock(ConfBlockDirective& b, ContextKey key);
		static ConfDirective&			getDirective(ConfBlockDirective& b, DirectiveKey key);

};