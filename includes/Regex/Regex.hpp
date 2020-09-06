#pragma once

#include "header.h"

#include "NFA.hpp"
#include "PatternValidation.hpp"

class Regex
{
	private:
		std::string			pattern;
		size_t				index;

		bool				anchor_start;
		bool				anchor_end;

		std::vector<char>	alphabet;

	public:
		NFA			automaton;
		Regex(const std::string& pattern);
		~Regex();

		std::pair<bool, std::string> match(const std::string& str);
		void	setNextStates(
			NFAState *state,
			std::vector<NFAState*>& next_states,
			std::vector<NFAState*>& visited);

		NFA axiom();
		NFA expr();
		NFA term();
		NFA factor();
		NFA modifier(NFA atm);
		NFA quantify(NFA atm);
		NFA atom();
		NFA	bracket();
		NFA setof(bool include, std::vector<char>& set);
		void subsetof(std::vector<char>& set);
		NFA charset();

		char peek();
		char next();
		void eat(char token);
		bool more();
};