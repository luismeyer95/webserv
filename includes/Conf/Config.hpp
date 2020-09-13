#pragma once

#include "ConfBlockDirective.hpp"
#include <Regex.hpp>
#include <ErrorCodeException.hpp>

class Config
{
	public:
		Config(const std::string& conf_path);
		ConfBlockDirective&			mainContext();

		void	bindVirtualHost (
			const std::string& request_uri,
			const std::string& request_ip_host,
			const std::string& request_servname,
			unsigned short request_port
		);

		std::string	root();

		// static std::vector<std::string> getDirectiveValues(ConfBlockDirective& b, DirectiveKey key);
		static ConfBlockDirective&		getBlock(ConfBlockDirective& b, ContextKey key);
		static ConfDirective&			getDirective(ConfBlockDirective& b, DirectiveKey key);



	private:
		// MANAGER
		// maps server_names to server blocks
		// std::map<std::string, ConfBlockDirective>	vhost_map;
		ConfBlockDirective*							vhost_binding;
		void	bindVHostRoute(const std::string& request_uri);

		// void										setVirtualHostMap();

		// PARSER
		ConfBlockDirective			main;
		std::string					conf_path;
		std::string					conf_file;

		std::vector<std::string>	tokens;
		std::vector<int>			token_line_nb;
		size_t						token_index;

		std::map<std::string, ContextKey>	context_key_lookup;
		std::map<std::string, DirectiveKey> directive_key_lookup;

		bool isWhitespace(char c);
		bool isDelimiter(char c);
		void tokenizeConf(std::ifstream& in);

		ConfBlockDirective			context (
			int line_nb, ContextKey key, const std::vector<std::string>& prefixes
		);

		void						link(ConfBlockDirective *parent, ConfBlockDirective& block);

		ConfBlockDirective			buildBlock();
		
		std::vector<std::string>	locationPrefixes();

		ConfDirective				buildDirective();

		std::string 				eat(const std::string& pattern);
		bool						peek(const std::string& pattern);
		std::string					peek();
		std::string					next();
		bool						more();
		int							line();

};