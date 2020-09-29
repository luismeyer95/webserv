#include <Regex/NFA.hpp>

NFA::NFA(NFAState *start, NFAState *end)
	: start(start), end(end) {}
	
NFAState* NFA::deepCopy ( 
	std::vector<NFAState*>& all_states_target,
	std::vector<NFAState*>& all_states_copy,
	NFAState *state, NFAState*& copy_end
)
{
	NFAState* copy = new NFAState();

	copy->is_end = state->is_end;

	all_states_target.push_back(state);
	all_states_copy.push_back(copy);

	auto fetch = [&] (NFAState* item) {
		NFAState* nested;
		auto it = std::find (
			all_states_target.begin(),
			all_states_target.end(),
			item
		);
		if (it == all_states_target.end())
			nested = deepCopy(all_states_target, all_states_copy, item, copy_end);
		else
		{
			size_t index = std::distance(all_states_target.begin(), it);
			nested = all_states_copy[index];
			all_states_target.push_back(item);
			all_states_copy.push_back(nested);
		}
		return nested;
	};

	for (auto& t : state->transition)
		copy->addTransition(t.first, fetch(t.second));
	for (auto& eps : state->epsilon_transitions)
		copy->addEpsilonTransition(fetch(eps));

	if (copy->is_end)
		copy_end = copy;
	
	return copy;
}


NFA NFA::copyAutomaton(NFA nfa)
{
	NFA copy;
	NFAState* end = nullptr;

	std::vector<NFAState*> all_states_target;
	std::vector<NFAState*> all_states_copy;

	copy.start = deepCopy(all_states_target, all_states_copy, nfa.start, end);
	copy.end = end;

	return copy;
}

void NFA::deleteAutomaton(NFA nfa)
{
	if (nfa.start && nfa.end)
	{
		std::vector<NFAState*> all;
		NFA::findAllStates(nfa.start, all);
		for (auto& st : all)
		{
			delete st;
			st = nullptr;
		}
	}
}

void NFA::findAllStates(NFAState* state, std::vector<NFAState*>& set)
{
	set.push_back(state);
	for (auto& t : state->transition)
	{
		if (std::find(set.begin(), set.end(), t.second) == set.end())
			findAllStates(t.second, set);
	}
	for (auto& eps : state->epsilon_transitions)
		if (std::find(set.begin(), set.end(), eps) == set.end())
			findAllStates(eps, set);
}

NFA NFA::fromSymbol(char c)
{
	NFA nfa(new NFAState(false), new NFAState(true));
	nfa.start->addTransition(c, nfa.end);
	return nfa;
}

NFA NFA::fromSymbolSet(const std::vector<char>& set)
{
	NFA nfa(new NFAState(false), new NFAState(true));
	for (auto c : set)
		nfa.start->addTransition(c, nfa.end);
	return nfa;
}

NFA NFA::fromEpsilon()
{
	NFA nfa(new NFAState(false), new NFAState(true));
	nfa.start->addEpsilonTransition(nfa.end);
	return nfa;
}

NFA NFA::concat(NFA a, NFA b)
{
	a.end->addEpsilonTransition(b.start);
	a.end->is_end = false;
	return NFA(a.start, b.end);
}

NFA NFA::unify(NFA a, NFA b)
{
	NFA nfa(new NFAState(false), new NFAState(true));
	nfa.start->addEpsilonTransition(a.start);
	nfa.start->addEpsilonTransition(b.start);

	a.end->addEpsilonTransition(nfa.end);
	a.end->is_end = false;
	b.end->addEpsilonTransition(nfa.end);
	b.end->is_end = false;

	return nfa;
}

NFA NFA::closure(NFA a)
{
	NFA nfa(new NFAState(false), new NFAState(true));

	nfa.start->addEpsilonTransition(nfa.end);
	nfa.start->addEpsilonTransition(a.start);

	a.end->addEpsilonTransition(nfa.end);
	a.end->addEpsilonTransition(a.start);
	a.end->is_end = false;

	return nfa;
}

NFA NFA::quantify(int min, int max, NFA a)
{
	if (max == -1)
		return concat(quantify(min, min, copyAutomaton(a)), closure(a));

	if (min)
		return concat(a, quantify(min - 1, max - 1, copyAutomaton(a)));
	else if (max)
		return concat(questionmark(a), quantify(min, max - 1, copyAutomaton(a)));
	else
	{
		NFA::deleteAutomaton(a);
		return fromEpsilon();
	}
}

NFA NFA::plus(NFA a)
{
	return NFA::concat(a, NFA::closure(a));
}

NFA NFA::questionmark(NFA a)
{
	NFA nfa(new NFAState(false), new NFAState(true));

	nfa.start->addEpsilonTransition(nfa.end);
	nfa.start->addEpsilonTransition(a.start);
	a.end->addEpsilonTransition(nfa.end);
	a.end->is_end = false;

	return nfa;
}

void	capture_state(NFAState *state, int capture, std::vector<NFAState*>& visited)
{
	if (std::find(visited.begin(), visited.end(), state) != visited.end())
		return;
	visited.push_back(state);
	if (!state->epsilon_transitions.empty())
	{
		for (auto& s : state->epsilon_transitions)
			capture_state(s, capture, visited);
	}
	else
	{
		if (state->is_end || state->capture_tags.count(capture))
			return;
		state->addCaptureTag(capture);
		capture_state(state->transition.begin()->second, capture, visited);
	}
}

NFA NFA::capture(NFA nfa, int capture)
{
	std::vector<NFAState*> visited;
	capture_state(nfa.start, capture, visited);
	return nfa;
}