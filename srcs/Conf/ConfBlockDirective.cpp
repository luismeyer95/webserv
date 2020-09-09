#include <Conf/ConfBlockDirective.hpp>

std::map<std::string, ContextKey> contextKeyLookup()
{
	using C = ContextKey;
	return std::map<std::string, ContextKey>
	({
		{"main", C::main},
		{"server", C::server},
		{"location", C::location}
	});
}

std::string contextKeyToString(ContextKey key)
{
	using C = ContextKey;
	switch (key)
	{
		case C::main: return "main";
		case C::server: return "server";
		case C::location: return "location";
	}
}

ConfBlockDirective::ConfBlockDirective() {}

ConfBlockDirective::ConfBlockDirective(ContextKey key, const std::vector<std::string>& prefixes)
	: key(key), prefixes(prefixes) {}

void ConfBlockDirective::validate()
{
	using C = ContextKey;
	switch (key)
	{
		case C::main: {
			validateMain();
			break;
		}
		case C::server: {
			validateServer();
			break;
		}
		case C::location: {
			validateLocation();
			break;
		}
		default: break;
	}
}

void ConfBlockDirective::validateMain()
{
	// ensure at least one server block is present
	auto it = std::find_if ( 
		blocks.begin(), blocks.end(),
		[] (const ConfBlockDirective& b) {
			return b.key == ContextKey::server;
		}
	);
	if (it == blocks.end())
		throw std::runtime_error("ConfParser: Main context has no server blocks");
	for (auto& nested : blocks)
		nested.validate();
}

void ConfBlockDirective::validateServer()
{
	auto it = std::find_if ( 
		blocks.begin(), blocks.end(),
		[] (const ConfBlockDirective& b) {
			return b.key != ContextKey::location;
		}
	);
	if (it != blocks.end())
		throw std::runtime_error(
			"ConfParser: Block directives inside server blocks "
			"should be of type `location`"
		);
	for (auto& nested : blocks)
		nested.validate();
}

void ConfBlockDirective::validateLocation()
{
	
}