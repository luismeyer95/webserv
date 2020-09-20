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
};

class RequestRouter
{
	friend class ServerSocketPool;
	private:
		std::shared_ptr<ConfBlockDirective> main;
		ConfBlockDirective*			route_binding;
		void		bindServer (
			const std::string& request_servname,
			const std::string& request_ip_host,
			unsigned short request_port
		);
		bool		bindLocation(const std::string& request_uri);

	public:
		RequestRouter();
		RequestRouter(const Config& conf);
		RequestRouter& operator=(const Config& conf);

		FileRequest	requestFile (
			const std::string&	request_uri,
			const std::string&	request_servname,
			const std::string&	request_ip_host,
			unsigned short		request_port
		);

		void fetchErrorPage(FileRequest& file_req);
		void fetchFile(FileRequest& file_req, const std::string& request_uri);
		std::string resolveUriToLocalPath(const std::string& request_uri);
		std::string resolveAliasUri(const std::string& request_uri, ConfBlockDirective& block);


		std::vector<std::string>		getBoundRequestDirectiveValues(DirectiveKey dirkey);

		static ConfBlockDirective&		getBlock(ConfBlockDirective& b, ContextKey key);
		static ConfDirective&			getDirective(ConfBlockDirective& b, DirectiveKey key);

};