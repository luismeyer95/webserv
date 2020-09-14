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

		NFA 				axiom();
		NFA					expr();
		NFA					term();
		NFA					factor();
		NFA					modifier(NFA atm);
		NFA					quantify(NFA atm);
		NFA					atom();
		NFA					bracket();
		NFA					setof(bool include, std::vector<char>& set);
		void				subsetof(std::vector<char>& set);
		NFA					charset();
		
		char 				peek();
		char				next();
		void				eat(char token);
		bool				more();

		NFA					automaton;

		void				setNextStates(
			NFAState *state,
			std::vector<NFAState*>& next_states,
			std::vector<NFAState*>& visited
		) const;

	public:
		Regex();
		Regex(const std::string& pattern);
		// Regex& operator=(const Regex& o);
		// Regex(const Regex& o);
		~Regex();

		static std::string			escapeSymbols(const std::string& str);

		std::pair<bool, std::string> match(const std::string& str) const;
		std::pair<bool, std::string> matchIn(
			const std::string& str,
			const std::string& before,
			const std::string& after
		);


};