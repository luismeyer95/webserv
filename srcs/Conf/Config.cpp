#include <Conf/Config.hpp>

Config::Config(const std::string& conf_path)
	: conf_path(conf_path), conf_file(), token_index(0),
	context_key_lookup(contextKeyLookup()),
	directive_key_lookup(directiveKeyLookup())
{
	tokens.reserve(128);

	std::ifstream in(conf_path);
	if (in.is_open())
	{
		tokenizeConf(in);
		in.close();
		try {
			main = std::make_shared<ConfBlockDirective>(context(1, ContextKey::main, {}));
			link(nullptr, *main);
			main->validate();
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
		bool open_quote = false;
		for (auto& c : line)
		{	
			// whitespace or delim
			if (!open_quote && (isWhitespace(c) || isDelimiter(c)))
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
			else if (c == '\"')
				open_quote = !open_quote;
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

	// for (auto& s : tokens)
	// 	std::cout << s << std::endl;
	// exit(0);
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
