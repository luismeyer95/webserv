#pragma once

#include "header.h"


class PatternValidation
{
	private:
		std::string			pattern;
		size_t				index;

		std::vector<char>	alphabet;

	public:
		PatternValidation(const std::string& pattern);
		~PatternValidation();

		void axiom();
		void expr();
		void term();
		void factor();
		void modifier();
		void quantify();
		void atom();
		void bracket();
		void setof();
		void subsetof();
		void charset();

		char peek();
		char next();
		void eat(char token);
		bool more();
};