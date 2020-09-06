#include <Regex/NFAState.hpp>

NFAState::NFAState(bool is_end) : is_end(is_end) {}

void NFAState::addEpsilonTransition(NFAState *state)
{
	epsilon_transitions.push_back(state);
}

void NFAState::addTransition(char c, NFAState *state)
{
	transition[c] = state;
}

NFAState* NFAState::transit(char c)
{
	if (transition.count(c))
		return transition[c];
	else
		return nullptr;
}

