#pragma once

#include "header.h"

#include "NFAState.hpp"

class NFA
{
	private:
		static NFAState* deepCopy ( 
			std::vector<NFAState*>& all_states_target,
			std::vector<NFAState*>& all_states_copy,
			NFAState *state, NFAState*& copy_end
		);

	public:
		NFAState*	start;
		NFAState*	end;

		NFA(NFAState *start = nullptr, NFAState *end = nullptr);

		static NFA copyAutomaton(NFA nfa);
		static void deleteAutomaton(NFA nfa);

		static void findAllStates(NFAState* state, std::vector<NFAState*>& set);

		// Primitives
		static NFA fromSymbol(char c);
		static NFA fromSymbolSet(const std::vector<char>& set);
		static NFA fromEpsilon();

		// Operations
		static NFA concat(NFA a, NFA b);
		static NFA unify(NFA a, NFA b);
		static NFA closure(NFA a);
		static NFA quantify(int min, int max, NFA a);
		static NFA plus(NFA a);
		static NFA questionmark(NFA a);
		
};