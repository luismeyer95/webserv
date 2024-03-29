#pragma once

#include "ConfDirective.hpp"

enum class ContextKey
{
	main, server, location
};

std::map<std::string, ContextKey>	contextKeyLookup();
std::string							contextKeyToString(ContextKey key);

struct ConfBlockDirective
{
	public:
		int								line_nb;
		ConfBlockDirective				*parent;

		ContextKey						key;			// location
		std::vector<std::string>		prefixes;		// {"~", "\.(gif|png)$"}

		std::vector<ConfBlockDirective> blocks;
		std::vector<ConfDirective>		directives;

		ConfBlockDirective();
		ConfBlockDirective (
			int line_number, ContextKey key,
			const std::vector<std::string>& prefixes = {}
		);

		void validate();

	private:

		void validateMain();
		void validateServer();
		void validateLocation();
};