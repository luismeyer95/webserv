#pragma once

#include "header.h"

#include "NFA.hpp"
#include "PatternValidation.hpp"

class Regex
{
	private:
		std::string			pattern;
		size_t				index;

		int					capture_index;

		bool				anchor_start;
		bool				anchor_end;

		std::vector<char>	alphabet;

		NFA 				axiom();
		NFA					expr(int capture);
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
		~Regex();

		static std::string			escapeSymbols(const std::string& str);

		std::pair<bool, std::string> match(const std::string& str) const;
		std::pair<bool, std::vector<std::string> > matchAll(const std::string& str) const;

		void constructPath (
			const std::vector< std::vector<NFATransition> > & all_trans,
			std::vector<char>& path, std::vector<ft::set<int>>& path_tags
		) const ;

		std::string buildCapture (
			const std::vector<char>& path, const std::vector<ft::set<int>>& tags, int capture
		) const;

		std::vector<std::string> buildAllCaptures (
			const std::vector<char>& path, const std::vector<ft::set<int>>& tags
		) const;
};