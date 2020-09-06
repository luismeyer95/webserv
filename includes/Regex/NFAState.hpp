#pragma once

#include "header.h"

class NFAState
{
	public:
	
		NFAState(bool is_end = false);

		std::vector<NFAState*> epsilon_transitions;
		std::map<char, NFAState*> transition;
		bool is_end;

		// Link state to another to form automatas
		void addEpsilonTransition(NFAState *state);
		void addTransition(char c, NFAState *state);

		// Returns next state on match
		NFAState* transit(char c);
};