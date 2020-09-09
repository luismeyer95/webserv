#pragma once

#include "ConfBlockDirective.hpp"

class ConfParser
{
	public:
		ConfParser(const std::string& conf_path);
		const ConfBlockDirective&	mainContext() const;
	private:
		ConfBlockDirective			main;
		std::string					conf_path;
		std::string					conf_file;

		std::vector<std::string>	tokens;
		size_t						token_index;

		std::map<std::string, ContextKey>	context_key_lookup;
		std::map<std::string, DirectiveKey> directive_key_lookup;

		bool isWhitespace(char c);
		bool isDelimiter(char c);
		void tokenizeConf(std::ifstream& in);

		ConfBlockDirective			context(ContextKey key, const std::vector<std::string>& prefixes);

		ConfBlockDirective			buildBlock();
		std::vector<std::string>	locationPrefixes();

		ConfDirective				buildDirective();

		std::string 				eat(const std::string& pattern);
		bool						peek(const std::string& pattern);
		std::string					peek();
		std::string					next();
		bool						more();


};