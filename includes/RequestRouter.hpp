#pragma once

#include <Conf/Config.hpp>
#include <ByteBuffer.hpp>
#include <RequestParser.hpp>
#include <ContentNegotiation.hpp>
#include <SharedPtr.hpp>
#include <CGI.hpp>

class	ServerSocketPool;
struct	HTTPExchange;
class	CGI;

class RequestRouter
{
	friend class ServerSocketPool;
	private:
		SharedPtr<ConfBlockDirective> main;
		ConfBlockDirective*			route_binding;
		ConfBlockDirective*			saved_binding;

		std::string					dir_backup;
	public:
		RequestRouter();
		RequestRouter(const Config& conf);
		RequestRouter& operator=(const Config& conf);

		void		bindServer (
			const std::string& request_servname,
			const std::string& request_ip_host,
			unsigned short request_port
		);
		bool		bindLocation
			(FileRequest& file_req, RequestParser& parsed_request,
			const std::string& request_uri, const std::string& request_method);
		void		setLocationDir();
		bool		hasMethod(const std::string& method, ConfBlockDirective& location_block);
		bool		saveMostSpecificLocation (
			const std::string& request_uri, ConfBlockDirective*& most_specific_prefix_loc
		);

		void		saveBinding();
		void		loadBinding();

		FileRequest	requestFile (
			RequestParser&		parsed_request,
			HTTPExchange&		ticket
		);

		void		executeCGI (
			FileRequest&		file_req,
			RequestParser&		parsed_request,
			HTTPExchange&		ticket
		);

		std::map<EnvCGI, std::string>	setCGIEnv (
			FileRequest&		file_req,
			RequestParser&		parsed_request,
			HTTPExchange&		ticket
		);

		bool		assertOrError(bool expr, FileRequest& file_req, RequestParser& parsed_request, int code, const std::string& msg);

		void		fetchErrorPage(FileRequest& file_req, RequestParser& parsed_request, int code, const std::string& msg);
		void		fetchFile(FileRequest& file_req, RequestParser& parsed_request, const std::string& request_uri);
		void		setFileRequest(FileRequest& file_req, RequestParser& parsed_request, const std::string& filepath);

		void		putFile(FileRequest& file_req, RequestParser& parsed_request, const std::string& request_uri);
		std::string resolveUriToLocalPath(const std::string& request_uri);
		std::string resolveAliasUri(const std::string& request_uri, ConfBlockDirective& block);
		std::string expandCaptures(std::string to_expand, const std::vector<std::string>& match_groups);

		std::string	typemapValue();
		bool		negotiateURI(FileRequest& file_req, RequestParser& parsed_request, const std::string& request_uri);


		bool		checkAuthorization(FileRequest& file_req, RequestParser& parsed_request, const std::string& basic_auth);
		bool		checkMethod(RequestParser& parsed_request, FileRequest& file_req);
		bool		checkBodyLength(RequestParser& parsed_request, FileRequest& file_req);
		void		checkRedirect(RequestParser& parsed_request, HTTPExchange& ticket, FileRequest& file_req);
		bool		checkAutoindex(FileRequest& file_req, RequestParser& parsed_request, const std::string& path);
		bool		methodIsEither(const std::string& method, const std::vector<std::string>& list);
		std::string	getAuthUser(const std::string& basic_auth);

		std::vector<std::string>		getBoundRequestDirectiveValues(DirectiveKey dirkey);
		std::string 					getCurrentLocationPrefix();
		static ConfBlockDirective&		getBlock(ConfBlockDirective& b, ContextKey key);
		static ConfDirective&			getDirective(ConfBlockDirective& b, DirectiveKey key);

};

struct FileRequest
{
		int							http_code;
		ssize_t						content_length;
		std::string					content_location;
		std::string					content_language;
		std::string					content_type;
		std::string					redirect_uri;
		std::string					http_string;
		std::string					file_path;
		std::string					last_modified;
		std::string					realm;
		std::vector<std::string>	allowed_methods;
		SharedPtr<ResponseBuffer>	response_buffer;

	FileRequest()
		: http_code(200), content_length(-1){}
};