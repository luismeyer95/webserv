#include <RequestRouter.hpp>

RequestRouter::RequestRouter()
	: route_binding(nullptr)
{
	
}

RequestRouter::RequestRouter(const Config& conf)
	: main(conf.main), route_binding(conf.main.get())
{

}

RequestRouter& RequestRouter::operator=(const Config& conf)
{
	this->main = conf.main;
	route_binding = conf.main.get();
	return *this;
}


ConfBlockDirective&		RequestRouter::getBlock(ConfBlockDirective& b, ContextKey key)
{
	auto it = std::find_if (
		b.blocks.begin(),
		b.blocks.end(),
		[&] (ConfBlockDirective blk) { return blk.key == key; }
	);
	if (it == b.blocks.end())
		throw std::runtime_error("RequestRouter: could not locate requested element in structure");
	return *it;
}

ConfDirective&			RequestRouter::getDirective(ConfBlockDirective& b, DirectiveKey key)
{
	auto it = std::find_if (
		b.directives.begin(),
		b.directives.end(),
		[&] (ConfDirective dir) { return dir.key == key; }
	);
	if (it == b.directives.end())
		throw std::runtime_error("RequestRouter: could not locate requested element in structure");
	return *it;
}

void	RequestRouter::getBoundRequestErrorPage()
{
	// search for an error_page directive in the default server
	ConfBlockDirective& servblock = *route_binding;
	ConfDirective& errpage = getDirective(servblock, DirectiveKey::error_page);


}

ErrorCode	RequestRouter::bindRoute(const std::string& request_uri)
{
	ConfBlockDirective* most_specific_prefix_loc = nullptr;
	for (auto& block : route_binding->blocks)
	{
		if (block.key == ContextKey::location)
		{
			std::vector<std::string>& prefixes = block.prefixes;
			if (prefixes.at(0) == "=")
			{
				std::string expr = Regex::escapeSymbols(prefixes.at(1));
				auto res = Regex("^" + expr + "$").match(request_uri);
				if (res.first)
				{
					route_binding = &block;
					return ErrorCode(200, "OK");
				}
			}
			if (prefixes.at(0) != "~")
			{
				std::string expr = Regex::escapeSymbols(prefixes.at(0));
				auto res = Regex("^" + expr).match(request_uri);
				if (res.first && (!most_specific_prefix_loc ||
					prefixes.at(0).size() > most_specific_prefix_loc->prefixes.at(0).size()))
					most_specific_prefix_loc = &block;
			}
		}
	}
	for (auto& block : route_binding->blocks)
		if (block.key == ContextKey::location)
		{
			std::vector<std::string>& prefixes = block.prefixes;
			if (prefixes.at(0) == "~")
			{
				auto res = Regex(prefixes.at(1)).match(request_uri);
				if (res.first)
				{
					route_binding = &block;
					return ErrorCode(200, "OK");
				}
			}
		}
	if (most_specific_prefix_loc)
	{
		route_binding = most_specific_prefix_loc;
		return ErrorCode(200, "OK");
	}
	else
		return ErrorCode(404, "Not Found");
}

ErrorCode	RequestRouter::bindRequest (
	const std::string& request_uri,
	const std::string& request_servname,
	const std::string& request_ip_host,
	unsigned short request_port
)
{
	ConfBlockDirective* default_server = nullptr;
	for (auto& block : main->blocks)
	{
		if (block.key == ContextKey::server)
		{
			std::string host_port = RequestRouter::getDirective(block, DirectiveKey::listen).values.at(0);
			auto tokens = tokenizer(host_port, ':');
			std::string host = tokens.at(0);
			if (host == "localhost")
				host = "127.0.0.1";
			unsigned short port = std::stoi(tokens.at(1));
			if (request_ip_host == host && request_port == port)
			{
				if (!default_server)
					default_server = &block;
				std::vector<std::string> servnames;
				try {
					servnames = RequestRouter::getDirective(block, DirectiveKey::server_name).values;
					for (auto& name : servnames)
					{
						if (name == request_servname)
						{
							route_binding = &block;
							return bindRoute(request_uri);
						}
					}
				} catch (const std ::runtime_error& e) {}				
			}
		}
	}
	if (default_server)
	{
		route_binding = default_server;
		return bindRoute(request_uri);
	}
	return ErrorCode(500, "Internal Server Error");
}

std::vector<std::string>	RequestRouter::getBoundRequestDirectiveValues(DirectiveKey dirkey)
{
	ConfBlockDirective* tmp = route_binding;
	while (tmp)
	{
		try {
			auto dirvals = getDirective(*tmp, dirkey).values;
			return dirvals;
		} catch (const std::runtime_error& e) {}
		tmp = tmp->parent;
	}
	return {};
}