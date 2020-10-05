#pragma once

#include <Conf/Config.hpp>
#include <ByteBuffer.hpp>
#include <RequestParser.hpp>
#include <SharedPtr.hpp>
#include <CGI.hpp>

class	ServerSocketPool;
struct	HTTPExchange;
class	CGI;

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
		bool		saveMostSpecificLocation (
			const std::string& request_uri, ConfBlockDirective*& most_specific_prefix_loc
		);

		void		saveBinding();
		void		loadBinding();

		std::shared_ptr<ConfBlockDirective> main;
		ConfBlockDirective*			route_binding;

		ConfBlockDirective*			saved_binding;
	public:
		RequestRouter();
		RequestRouter(const Config& conf);
		RequestRouter& operator=(const Config& conf);

		FileRequest	requestFile (
			RequestParser&		parsed_request,
			HTTPExchange&		ticket
		);

		void		executeCGI(
			FileRequest&		file_req,
			RequestParser&		parsed_request,
			HTTPExchange&		ticket
		);

		std::map<EnvCGI, std::string>	setCGIEnv (
			FileRequest&		file_req,
			RequestParser&		parsed_request,
			HTTPExchange&		ticket
		);

		void fetchErrorPage(FileRequest& file_req, RequestParser& parsed_request, int code, const std::string& msg);
		void fetchFile(FileRequest& file_req, RequestParser& parsed_request, const std::string& request_uri);
		std::string resolveUriToLocalPath(const std::string& request_uri);
		std::string resolveAliasUri(const std::string& request_uri, ConfBlockDirective& block);

		bool		checkAuthorization(FileRequest& file_req, RequestParser& parsed_request, const std::string& basic_auth);
		bool		checkMethod(RequestParser& parsed_request, FileRequest& file_req);
		void		checkRedirect(RequestParser& parsed_request, HTTPExchange& ticket, FileRequest& file_req);

		std::string	getAuthUser(const std::string& basic_auth);



		std::vector<std::string>		getBoundRequestDirectiveValues(DirectiveKey dirkey);

		static ConfBlockDirective&		getBlock(ConfBlockDirective& b, ContextKey key);
		static ConfDirective&			getDirective(ConfBlockDirective& b, DirectiveKey key);

};

struct FileRequest
{
		std::string					redirect_uri;
		int							http_code;
		std::string					http_string;
		std::string					file_path;
		std::string					last_modified;
		// ByteBuffer					file_content;
		SharedPtr<ResponseBuffer>	response_buffer;
		std::string					content_type;
		ssize_t						content_length;
		std::vector<std::string>	allowed_methods;
		std::string					realm;

	FileRequest()
		: http_code(200), content_length(-1){}

	void print()
	{
		std::cout << "redirect_uri: " << redirect_uri << std::endl;
		std::cout << "http_code " << http_code<< std::endl;
		std::cout << "http_string: " << http_string << std::endl;
		std::cout << "file_path: " << file_path << std::endl;
		std::cout << "last_modified: " << last_modified << std::endl;
		std::cout << "content_type: " << content_type << std::endl;
		std::cout << "allowed_methods: ";
		for (auto& s : allowed_methods)
			std::cout << s << " ";
		std::cout << std::endl;
		std::cout << "realm: " << realm << std::endl;
	}
};