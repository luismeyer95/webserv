#pragma once

#include "header.h"
#include "Regex.hpp"

enum class ContextKey
{
	main, server, location
};

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

enum class DirectiveKey
{
	listen, server_name, root, error_page, internal, index
};

std::map<std::string, DirectiveKey> directiveKeyLookup()
{
	using D = DirectiveKey;
	return std::map<std::string, DirectiveKey>
	({
		{"listen", D::listen},
		{"server_name", D::server_name},
		{"root", D::root},
		{"error_page", D::error_page},
		{"internal", D::internal},
		{"index", D::index}
	});
}

std::string directiveKeyToString(DirectiveKey key)
{
	using D = DirectiveKey;
	switch (key)
	{
		case D::listen: return "listen";
		case D::server_name: return "server_name";
		case D::root: return "root";
		case D::error_page: return "error_page";
		case D::internal: return "internal";
		case D::index: return "index";
	}
}

struct ConfDirective
{
	DirectiveKey	key;					// server_name
	std::vector<std::string> values;		// {"example.org", "www.example.com"}

	ConfDirective(DirectiveKey key, const std::vector<std::string>& values)
		: key(key), values(values) {}
};

struct ConfBlockDirective
{
	public:

		ContextKey key;							// location
		std::vector<std::string> prefixes;		// {"~", "\.(gif|png)$"}

		std::vector<ConfBlockDirective> blocks;
		std::vector<ConfDirective> directives;

		ConfBlockDirective() {}
		ConfBlockDirective(ContextKey key, const std::vector<std::string>& prefixes = {})
			: key(key), prefixes(prefixes)
		{

		}

		void validate()
		{
			using C = ContextKey;
			switch (key)
			{
				case C::main: {
					validate_main();
					break;
				}
				case C::server: {
					validate_server();
					break;
				}
				case C::location: {
					validate_location();
					break;
				}
				default: break;
			}
		}

	private:
		void validate_main()
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

		void validate_server()
		{
			for (auto& nested : blocks)
				nested.validate();
		}

		void validate_location()
		{
			size_t index = 0;
			if (prefixes.empty())
				throw std::runtime_error("ConfBlockDirective: Empty location block prefixes");
			if (prefixes[index] == "~" || prefixes[index] == "~*")
			{
				if (prefixes.size() < 2)
					throw std::runtime_error("ConfBlockDirective: Location block with "
											 "regex modifier missing regex expression");
				++index;
				try {
					Regex rgx(prefixes[index]);
				} catch (const std::runtime_error& e) {
					std::string errstr = "ConfBlockDirective: Location block has invalid regex expression\n";
					errstr += e.what();
					throw std::runtime_error(errstr);
				}
			}
			else
			{
				// only match absolute paths without ".." or "." path segments
				// UPDATE THIS REGEX PATTERN -  MAY BE TOO RESTRICTIVE
				if (prefixes[index] == "=")
					++index;
				bool is_valid_uri = Regex("^/|(/[-_a-zA-Z\\d]+(\\.[-_a-zA-Z\\d]+)?)+/?$").match(prefixes[index]).first;
				if (!is_valid_uri)
				{
					throw std::runtime_error (
						"ConfParser: Invalid URI in `"
						+ contextKeyToString(key) + "` block prefix"
					);
				}
			}
		}
};


class ConfParser
{
	private:
		ConfBlockDirective			main;
		std::string					conf_path;
		std::string					conf_file;

		std::map<std::string, ContextKey> context_key_lookup;
		std::map<std::string, DirectiveKey> directive_key_lookup;

		std::vector<std::string>	tokens;
		size_t						token_index;

	public:
		ConfParser(const std::string& conf_path)
			: conf_path(conf_path),
			  conf_file(), context_key_lookup(contextKeyLookup()),
			  directive_key_lookup(directiveKeyLookup()), token_index(0)
		{
			(void)token_index;

			std::ifstream in(conf_path);
			if (in.is_open())
			{
				std::stringstream stream;
				std::string temp;
				while(getline(in, temp, '\n'))
				{
					temp = temp.substr(0, temp.find('#'));
					stream << temp << '\n';
				}
				conf_file = stream.str();
				tokenizeConfFile();
				in.close();
				main = context(ContextKey::main, {});
			}
			else
				throw std::runtime_error("ConfParser: Could not open configuration file");
		}

		const ConfBlockDirective& mainContext() const
		{
			return main;
		}

		void tokenizeConfFile()
		{
			std::string buf;
			buf.reserve(512);
			for (auto& c : conf_file)
			{	
				// whitespace
				if ((c >= 9 && c <= 13) || c == 32 || c == ';')
				{
					if (!buf.empty())
						tokens.push_back(buf);
					if (c == ';')
						tokens.push_back(";");
					buf.clear();
				}
				else
					buf.push_back(c);
			}
			if (!buf.empty())
				tokens.push_back(buf);

			for (auto& tk : tokens)
				std::cout << tk << std::endl;
		}

		ConfBlockDirective context(ContextKey key, const std::vector<std::string>& prefixes)
		{
			ConfBlockDirective block(key, prefixes);

			while (more() && peek() != "}")
			{
				if (peek("^(server|location)$"))
					setBlock(block);
				else if (peek("^(listen|server_name|root|error_page|index|internal)$"))
					setDirective(block);
				else
					throw std::runtime_error("ConfParser: Unexpected token `" + peek() + "`");
			}
			if (key != ContextKey::main)
			{
				if (peek() != "}")
				{
					throw std::runtime_error (
						"ConfParser: Missing closing curly bracket on `"
						+ contextKeyToString(key) + "` block directive"
					);
				}
				next();
			}
			else
				block.validate();
			return block;
		}

		void setBlock(ConfBlockDirective& block)
		{
			std::string strkey = next();
			ContextKey nested_block_key = context_key_lookup[strkey];

			std::vector<std::string> nested_prefixes;
			while (more() && peek() != "{")
				nested_prefixes.push_back(next());
			if (more())
				eat("^{$");

			block.blocks.push_back(context(nested_block_key, nested_prefixes));
		}

		void setDirective(ConfBlockDirective& block)
		{
			std::string strkey = next();
			DirectiveKey directive_key = directive_key_lookup[strkey];
			std::vector<std::string> values;
			while (more() && peek() != ";")
				values.push_back(next());
			if (more())
				eat("^;$");

			block.directives.push_back(ConfDirective(directive_key, values));
		}

		std::string eat(const std::string& pattern)
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

		bool peek(const std::string& pattern)
		{
			if (!more())
				return false;
			auto res = Regex(pattern).match(tokens[token_index]);
			return res.first;
		}

		std::string peek()
		{
			if (!more())
				return "";
			return tokens[token_index];
		}

		std::string next()
		{
			return tokens[token_index++];
		}

		bool more()
		{
			return token_index < tokens.size();
		}

};