#include <Conf/ConfParser.hpp>

ConfParser::ConfParser(const std::string& conf_path)
	: conf_path(conf_path), conf_file(), token_index(0),
	context_key_lookup(contextKeyLookup()),
	directive_key_lookup(directiveKeyLookup())
{
	// (void)token_index;

	std::ifstream in(conf_path);
	if (in.is_open())
	{
		tokenizeConf(in);
		in.close();
		main = context(ContextKey::main, {});
	}
	else
		throw std::runtime_error("ConfParser: Could not open configuration file");
}

bool ConfParser::isWhitespace(char c)
{
	return (c >= 9 && c <= 13) || c == 32;
}

bool ConfParser::isDelimiter(char c)
{
	return c == ';' || c == '{' || c == '}';
}

void ConfParser::tokenizeConf(std::ifstream& in)
{
	std::stringstream stream;
	std::string temp;
	while(getline(in, temp, '\n'))
	{
		temp = temp.substr(0, temp.find('#'));
		stream << temp << '\n';
	}
	conf_file = stream.str();

	std::string buf;
	buf.reserve(128);
	for (auto& c : conf_file)
	{	
		// whitespace or delim
		if (isWhitespace(c) || isDelimiter(c))
		{
			if (!buf.empty())
				tokens.push_back(buf);
			if (isDelimiter(c))
				tokens.push_back(std::string(1, c));
			buf.clear();
		}
		else
			buf.push_back(c);
	}
	if (!buf.empty())
		tokens.push_back(buf);
	// for (auto& tk : tokens)
	// 	std::cout << tk << std::endl;
}

ConfBlockDirective ConfParser::context(ContextKey key, const std::vector<std::string>& prefixes)
{
	ConfBlockDirective block(key, prefixes);

	while (more() && peek() != "}")
	{
		if (context_key_lookup.count(peek()))
			block.blocks.push_back(buildBlock());
		else if (directive_key_lookup.count(peek()))
			block.directives.push_back(buildDirective());
		else
			throw std::runtime_error("ConfParser: Unexpected token `" + peek() + "`");
	}
	if (key != ContextKey::main)
	{
		if (peek() != "}")
		{
			throw std::runtime_error (
				"ConfParser: Missing closing curly brace on `"
				+ contextKeyToString(key) + "` block directive"
			);
		}
		next();
	}
	else
		block.validate();
	return block;
}

ConfBlockDirective ConfParser::buildBlock()
{
	std::string strkey = next();
	ContextKey nested_block_key = context_key_lookup[strkey];

	std::vector<std::string> nested_prefixes;
	switch (nested_block_key)
	{
		case ContextKey::location:
			nested_prefixes = locationPrefixes();
		default: break;
	}

	if (next() != "{")
		throw std::runtime_error (
			"ConfParser: Missing opening curly brace for `"
			+ strkey + "` block directive"
		);
		
	return context(nested_block_key, nested_prefixes);
}

std::vector<std::string> ConfParser::locationPrefixes()
{
	std::vector<std::string> prefixes;
	if (peek() == "~" || peek() == "~*")
	{
		prefixes.push_back(next());
		try {
			Regex rgx(peek());
		} catch (const std::runtime_error& e) {
			std::string errstr = "ConfBlockDirective: Location block has invalid regex expression\n";
			errstr += e.what();
			throw std::runtime_error(errstr);
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
			throw std::runtime_error (
				"ConfParser: Invalid URI in `location` block prefix"
			);
		}
		prefixes.push_back(next());

	}
	return prefixes;
}

ConfDirective ConfParser::buildDirective()
{
	std::string strkey = next();
	DirectiveKey directive_key = directive_key_lookup[strkey];
	std::vector<std::string> values;
	while (more() && peek() != ";")
		values.push_back(next());
	// if (more())
		eat("^;$");

	return ConfDirective(directive_key, values);
}

std::string ConfParser::eat(const std::string& pattern)
{
	if (!more())
		throw std::runtime_error("ConfParser: Unexpected end of config file");
	auto res = Regex(pattern).match(tokens[token_index]);
	if (!res.first)
	{
		std::string err = "ConfParser: Unexpected token `" + tokens[token_index] + '`'; 
		throw std::runtime_error(err);
	}
	else
	{
		token_index++;
		return res.second;
	}				
}

bool ConfParser::peek(const std::string& pattern)
{
	if (!more())
		return false;
	auto res = Regex(pattern).match(tokens[token_index]);
	return res.first;
}

std::string ConfParser::peek()
{
	if (!more())
		return "";
	return tokens[token_index];
}

std::string ConfParser::next()
{
	return tokens[token_index++];
}

bool ConfParser::more()
{
	return token_index < tokens.size();
}

const ConfBlockDirective& ConfParser::mainContext() const
{
	return main;
}