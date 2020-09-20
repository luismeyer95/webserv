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

ConfBlockDirective::ConfBlockDirective()
	: parent(nullptr) {}

ConfBlockDirective::ConfBlockDirective (
	int line_number, ContextKey key,
	const std::vector<std::string>& prefixes
) : line_nb(line_number), parent(nullptr), key(key), prefixes(prefixes) {}

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
		throw ConfError(line_nb, "main context has no server blocks");
	for (auto& dir : directives)
		dir.validate();
	for (auto& nested : blocks)
		nested.validate();
}

void ConfBlockDirective::validateServer()
{
	// ensure at least one nested block is present
	if (blocks.empty())
		throw ConfError (
			line_nb,
			"`server` blocks require at least one `location` block directive"
		);
	// ensure no nested blocks other than location blocks are present
	auto it = std::find_if ( 
		blocks.begin(), blocks.end(),
		[] (const ConfBlockDirective& b) {
			return b.key != ContextKey::location;
		}
	);
	if (it != blocks.end())
		throw ConfError(
			line_nb,
			"only `location` block directives are allowed inside server blocks"
		);
	
	// validate directives and nested blocks
	for (auto& dir : directives)
		dir.validate();
	for (auto& nested : blocks)
		nested.validate();
}

void ConfBlockDirective::validateLocation()
{
	// ensure it has a root/alias directive
	ConfBlockDirective *b = this;
	while (b)
	{
		auto count = std::count_if (
			b->directives.begin(), b->directives.end(),
			[] (const ConfDirective& d) {
				return d.key == DirectiveKey::root
				|| d.key == DirectiveKey::alias;
			}
		);
		if (count == 1)
		{
			for (auto& dir : directives)
				dir.validate();
			return;
		}
		else if (count > 1)
		{
			throw ConfError (
				line_nb,
				"alias/root directive conflict for `location` block"
			);
		}
		b = b->parent;
	}
	throw ConfError (
		line_nb,
		"`location` block requires either an alias or a root directive"
	);
}