#pragma once

#include "header.h"

class NFAState;

class NFATransition
{
	public:
		ft::set<int>	capture_tags;
		NFAState*		start_state;
		NFAState*		transition_state;
		char			symbol;

		NFATransition(NFAState *start, char symbol, NFAState *end, const ft::set<int>& capture_tags)
			: capture_tags(capture_tags), start_state(start), transition_state(end), symbol(symbol) {}
		
		// Null transition
		NFATransition()
			: capture_tags(), start_state(nullptr), transition_state(nullptr), symbol(0) {}

		bool operator<(const NFATransition& o)
		{
			return start_state < o.start_state;
		}
};

class NFAState
{
	public:
	
		NFAState(bool is_end = false);

		std::vector<NFAState*>		epsilon_transitions;
		std::map<char, NFAState*>	transition;
		ft::set<int>				capture_tags;
		bool is_end;

		// Link state to another to form automatas
		void addEpsilonTransition(NFAState *state);
		void addTransition(char c, NFAState *state);
		void addCaptureTag(int capture);

		// Returns next state on match
		// NFAState* transit(char c);
		std::vector<NFATransition> transit(char c, std::vector<NFAState*>& next_states);

		void	setNextStates (
			NFAState *state,
			std::vector<NFAState*>& next_states,
			std::vector<NFAState*>& visited
		) const;
};