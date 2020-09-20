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

void NFAState::addCaptureTag(int capture)
{
	capture_tags.insert(capture);
}

void	NFAState::setNextStates(
	NFAState *state,
	std::vector<NFAState*>& next_states,
	std::vector<NFAState*>& visited) const
{
	if (state->epsilon_transitions.size())
	{
		for (auto& st : state->epsilon_transitions)
		{
			// if state has not been visited
			if (std::find_if (
					visited.begin(), visited.end(),
					[&st] (NFAState* vs) { return vs == st; }
				) == visited.end())
			{
				visited.push_back(st);
				setNextStates(st, next_states, visited);
			}
		}
	}
	else
		next_states.push_back(state);
}

std::vector<NFATransition> NFAState::transit(char c, std::vector<NFAState*>& next_states)
{
	if (transition.count(c))
	{
		std::vector<NFAState*> visited;
		std::vector<NFAState*> next_current;

		NFAState* next = transition[c];
		setNextStates(next, next_current, visited);

		std::vector<NFATransition> trans_set;
		for (auto& st : next_current)
			trans_set.push_back(NFATransition(this, c, st, capture_tags));

		next_states.insert(next_states.end(), next_current.begin(), next_current.end());

		return trans_set;
	}
	else
		return {};
}

