#pragma once

#include <Conf/Config.hpp>
#include <ErrorCode.hpp>

class ServerSocketPool;

class RequestRouter
{
	friend class ServerSocketPool;
	private:
		std::shared_ptr<ConfBlockDirective> main;
		ConfBlockDirective*			route_binding;
		ErrorCode					bindRoute(const std::string& request_uri);
		void						handleRouteNotFound();

	public:
		RequestRouter();
		RequestRouter(const Config& conf);
		RequestRouter& operator=(const Config& conf);

		ErrorCode	bindRequest (
			const std::string&	request_uri,
			const std::string&	request_servname,
			const std::string&	request_ip_host,
			unsigned short		request_port
		);

		std::vector<std::string>		getBoundRequestDirectiveValues(DirectiveKey dirkey);
		void							getBoundRequestErrorPage();

		static ConfBlockDirective&		getBlock(ConfBlockDirective& b, ContextKey key);
		static ConfDirective&			getDirective(ConfBlockDirective& b, DirectiveKey key);

};