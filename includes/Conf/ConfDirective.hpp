#pragma once

#include <header.h>
#include <Regex.hpp>

enum class DirectiveKey
{
	listen, server_name, root, error_page,
	internal, index, autoindex
};

std::map<std::string, DirectiveKey> directiveKeyLookup();
std::string							directiveKeyToString(DirectiveKey key);

struct ConfDirective
{
	public:
		DirectiveKey				key;		// server_name
		std::vector<std::string>	values;		// {"example.org", "www.example.com"}

		ConfDirective(DirectiveKey key, const std::vector<std::string>& values);

		void validate();

	private:

};