#include <Conf/Config.hpp>

Config::Config(const std::string& conf_path)
	: vhost_binding(nullptr), conf_path(conf_path), conf_file(), token_index(0),
	context_key_lookup(contextKeyLookup()),
	directive_key_lookup(directiveKeyLookup())
{
	// (void)token_index;
	tokens.reserve(128);

	std::ifstream in(conf_path);
	if (in.is_open())
	{
		tokenizeConf(in);
		in.close();
		try {
			main = context(1, ContextKey::main, {});
			link(nullptr, main);
			main.validate();
			vhost_binding = &main;
			// setVirtualHostMap();
		} catch (const ConfError& e) {
			throw std::runtime_error (
				conf_path + ": line " + std::to_string(e.line())
				+ ": " + e.err()
			);
		}
	}
	else
		throw std::runtime_error("could not open configuration file");
}

bool Config::isWhitespace(char c)
{
	return (c >= 9 && c <= 13) || c == 32;
}

bool Config::isDelimiter(char c)
{
	return c == ';' || c == '{' || c == '}';
}

void Config::tokenizeConf(std::ifstream& in)
{
	std::stringstream stream;
	std::string line;

	int line_nb = 1;
	while (getline(in, line, '\n'))
	{
		line = line.substr(0, line.find('#'));
		stream << line << '\n';

		std::string buf;
		buf.reserve(128);
		for (auto& c : line)
		{	
			// whitespace or delim
			if (isWhitespace(c) || isDelimiter(c))
			{
				if (!buf.empty())
				{
					tokens.push_back(buf);
					token_line_nb.push_back(line_nb);
				}
				if (isDelimiter(c))
				{
					tokens.push_back(std::string(1, c));
					token_line_nb.push_back(line_nb);
				}
				buf.clear();
			}
			else
				buf.push_back(c);
		}
		if (!buf.empty())
		{
			tokens.push_back(buf);
			token_line_nb.push_back(line_nb);
		}
		
		line_nb++;
	}
	// for (size_t i = 0; i < tokens.size(); ++i)
	// 	std::cout << tokens[i] << " (line " << token_line_nb[i] << ")" << std::endl;
}

ConfBlockDirective Config::context (
	int line_nb, ContextKey key, const std::vector<std::string>& prefixes
)
{
	ConfBlockDirective block(line_nb, key, prefixes);

	while (more() && peek() != "}")
	{
		if (context_key_lookup.count(peek()))
			block.blocks.push_back(buildBlock());
		else if (directive_key_lookup.count(peek()))
			block.directives.push_back(buildDirective());
		else
			throw ConfError(line(), "unexpected token `" + peek() + "`");
	}
	if (key != ContextKey::main)
	{
		if (peek() != "}")
		{
			throw ConfError (
				line_nb,
				"missing closing curly brace on `"
				+ contextKeyToString(key) + "` block directive"
			);
		}
		next();
	}
	return block;
}

void Config::link(ConfBlockDirective *parent, ConfBlockDirective& block)
{
	block.parent = parent;
	for (auto& dir : block.directives)
		dir.parent = &block;
	for (auto& b : block.blocks)
		link(&block, b);
}


ConfBlockDirective Config::buildBlock()
{
	int line_nb = line();
	std::string strkey = next();
	ContextKey nested_block_key = context_key_lookup[strkey];

	std::vector<std::string> nested_prefixes;
	switch (nested_block_key)
	{
		case ContextKey::location:
		{
			nested_prefixes = locationPrefixes();
			break;
		}
		default: break;
	}

	if (peek() != "{")
		throw ConfError (
			line_nb,
			"missing opening curly brace for `"
			+ strkey + "` block directive"
		);
	next();
		
	return context(line_nb, nested_block_key, nested_prefixes);
}

std::vector<std::string> Config::locationPrefixes()
{
	std::vector<std::string> prefixes;
	if (peek() == "~" || peek() == "~*")
	{
		prefixes.push_back(next());
		try {
			Regex rgx(peek());
		} catch (const std::runtime_error& e) {
			throw ConfError (
				line(),
				"`location` block regex expression is invalid. "
				+ std::string(e.what())
			);
		}
		prefixes.push_back(next());
	}
	else
	{
		if (peek() == "=")
			prefixes.push_back(next());

		// Only match absolute paths without ".." or "." path segments
		// UPDATE THIS REGEX PATTERN -  MAY BE TOO RESTRICTIVE
		bool valid_uri = Regex("^/|(/[-_a-zA-Z\\d]+(\\.[-_a-zA-Z\\d]+)?)+/?$").match(peek()).first;
		if (!valid_uri)
		{
			throw ConfError (
				line(),
				"invalid URI in `location` block prefix"
			);
		}
		prefixes.push_back(next());

	}
	return prefixes;
}

ConfDirective Config::buildDirective()
{
	int line_nb = line();
	std::string strkey = next();
	DirectiveKey directive_key = directive_key_lookup[strkey];

	std::vector<std::string> values;
	while (more() && peek() != ";")
		values.push_back(next());
	// if (more())
	eat("^;$");

	return ConfDirective(line_nb, directive_key, values);
}

std::string Config::eat(const std::string& pattern)
{
	if (!more())
		throw ConfError(token_line_nb.back(), "unexpected end of config file");
	auto res = Regex(pattern).match(peek());
	if (!res.first)
		throw ConfError(line(), "unexpected token `" + peek() + '`');
	else
	{
		token_index++;
		return res.second;
	}				
}

bool Config::peek(const std::string& pattern)
{
	if (!more())
		return false;
	auto res = Regex(pattern).match(tokens[token_index]);
	return res.first;
}

std::string Config::peek()
{
	if (!more())
		return "";
	return tokens[token_index];
}

std::string Config::next()
{
	return tokens[token_index++];
}

bool Config::more()
{
	return token_index < tokens.size();
}

int Config::line()
{
	if (token_line_nb.empty())
		return 1;
	if (!more())
		return token_line_nb.back();
	return token_line_nb[token_index];
}

ConfBlockDirective& Config::mainContext()
{
	return main;
}

ConfBlockDirective&		Config::getBlock(ConfBlockDirective& b, ContextKey key)
{
	auto it = std::find_if (
		b.blocks.begin(),
		b.blocks.end(),
		[&] (ConfBlockDirective blk) { return blk.key == key; }
	);
	if (it == b.blocks.end())
		throw std::runtime_error("Config: could not locate requested element in configuration structure");
	return *it;
}

ConfDirective&			Config::getDirective(ConfBlockDirective& b, DirectiveKey key)
{
	auto it = std::find_if (
		b.directives.begin(),
		b.directives.end(),
		[&] (ConfDirective dir) { return dir.key == key; }
	);
	if (it == b.directives.end())
		throw std::runtime_error("Config: could not locate requested element in configuration structure");
	return *it;
}

void	Config::bindVHostRoute(const std::string& request_uri)
{
	ConfBlockDirective* most_specific_prefix_loc = nullptr;
	for (auto& block : vhost_binding->blocks)
	{
		if (block.key == ContextKey::location)
		{
			std::vector<std::string>& prefixes = block.prefixes;
			if (prefixes.at(0) != "~")
			{
				auto res = Regex("^" + prefixes.at(0)).match(request_uri);
				if (res.first && (!most_specific_prefix_loc ||
					prefixes.at(0).size() > most_specific_prefix_loc->prefixes.at(0).size()))
					most_specific_prefix_loc = &block;
			}
		}
	}
	for (auto& block : vhost_binding->blocks)
	{
		if (block.key == ContextKey::location)
		{
			std::vector<std::string>& prefixes = block.prefixes;
			if (prefixes.at(0) == "~")
			{
				auto res = Regex(prefixes.at(1)).match(request_uri);
				if (res.first)
				{
					vhost_binding = &block;
					return;
				}
			}
		}
	}
	if (most_specific_prefix_loc)
		vhost_binding = most_specific_prefix_loc;
	else
		throw ErrorCodeException(404, "Not Found");
}

void	Config::bindVirtualHost (
	const std::string& request_uri,
	const std::string& request_ip_host,
	const std::string& request_servname,
	unsigned short request_port
)
{
	ConfBlockDirective* default_server = nullptr;
	for (auto& block : mainContext().blocks)
	{
		if (block.key == ContextKey::server)
		{
			std::string host_port = Config::getDirective(block, DirectiveKey::listen).values.at(0);
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
					servnames = Config::getDirective(block, DirectiveKey::server_name).values;
					for (auto& name : servnames)
					{
						if (name == request_servname)
						{
							vhost_binding = &block;
							bindVHostRoute(request_uri);
							return;
						}
					}
				} catch (const std ::runtime_error& e) {}				
			}
		}
	}
	if (default_server)
	{
		vhost_binding = default_server;
		bindVHostRoute(request_uri);
	}
	else
		throw ErrorCodeException(404, "Not Found");
}

std::string	Config::root()
{
	ConfBlockDirective* tmp = vhost_binding;
	while (tmp)
	{
		try {
			std::string root_path = getDirective(*tmp, DirectiveKey::root).values.at(0);
			return root_path;
		} catch (const std::runtime_error& e) {}
		tmp = tmp->parent;
	}
	return "null";
}
