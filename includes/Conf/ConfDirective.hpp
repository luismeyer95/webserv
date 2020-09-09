#pragma once

#include <header.h>
#include <Regex.hpp>


class ConfError : public std::exception
{
	private:
		int			err_line;
		std::string err_str;
	public:
		ConfError(int line, const std::string& str)
			: err_line(line), err_str(str) {}
		
		std::string err() const throw()
		{
			return err_str;
		}

		int line() const throw()
		{
			return err_line;
		}

};

enum class DirectiveKey
{
	listen, server_name, root, error_page,
	internal, index, autoindex
};

std::map<std::string, DirectiveKey> directiveKeyLookup();
std::string							directiveKeyToString(DirectiveKey key);

struct ConfBlockDirective;

struct ConfDirective
{
	public:
		int							line_nb;
		ConfBlockDirective			*parent;

		DirectiveKey				key;		// server_name
		std::vector<std::string>	values;		// {"example.org", "www.example.com"}

		ConfDirective (
			int line_number, DirectiveKey key,
			const std::vector<std::string>& values
		);

		void						validate();

	private:
		ConfError					missingVal();

};