#include <Regex/Regex.hpp>

Regex::Regex() {}

Regex::Regex(const std::string& pattern)
	: pattern(pattern), index(0), capture_index(0),
	anchor_start(false), anchor_end(false), automaton()
{
	PatternValidation check(pattern);
	
	alphabet.reserve(256);
	for (int i = 0; i < 256; ++i)
		alphabet.push_back(static_cast<char>(i));

	automaton = axiom();
}

Regex::~Regex()
{
	NFA::deleteAutomaton(automaton);
}

std::string Regex::escapeSymbols(const std::string& str)
{
	std::string ret;
	std::string symbolset("^$[]{}()*+?.\\|-");

	ret.reserve(str.capacity());
	for (auto& c : str)
	{
		if (symbolset.find(c) != std::string::npos)
			ret += "\\";
		ret += c;
	}
	return ret;
}

std::pair<bool, std::string> Regex::match(const std::string& str) const
{
	std::vector<NFAState*> current_states;
	std::vector<NFAState*> visited;

	setNextStates(automaton.start, current_states, visited);
	visited.clear();

	std::vector<NFAState*> next_states;
	for (auto& c : str)
	{
		next_states.clear();
		for (auto& state : current_states)
			state->transit(c, next_states);
		current_states = next_states;

		if (!anchor_end)
		{
			// if end of string is not anchored, return true
			// as soon as an accepting state is found
			auto it = std::find_if (
				current_states.begin(), current_states.end(),
				[] (NFAState *st) { return st->is_end; }
			);
			if (it != current_states.end())
			{
				size_t len = (&c - &str[0]) + 1;
				return {true, str.substr(0, len)};
			}
		}
	}

	auto it = std::find_if (
		current_states.begin(), current_states.end(),
		[] (NFAState *st) { return st->is_end; }
	);

	// if no start anchor set, try to match again for next char until
	// end of string
	bool found = it != current_states.end();
	if (!found && !anchor_start && !str.empty())
	{
		std::pair<bool, std::string> next_match = match(&str[0] + 1);
		return {found || next_match.first, next_match.second };
	}

	return {found, str};
}

std::pair<bool, std::vector<std::string> > Regex::matchAll(const std::string& str) const
{
	std::vector<char>			path;
	std::vector<ft::set<int>>	path_tags;

	std::vector< std::vector<NFATransition> > all_trans;
	all_trans.reserve(str.size());
	
	std::vector<NFAState*> current_states;
	std::vector<NFAState*> visited;
	setNextStates(automaton.start, current_states, visited);
	// visited.clear();

	std::vector<NFAState*> next_states;
	for (auto& c : str)
	{
		std::vector<NFATransition> trans_symbol;
		next_states.clear();
		for (auto& state : current_states)
		{
			std::vector<NFATransition> trans = state->transit(c, next_states);
			std::move(trans.begin(), trans.end(), std::back_inserter(trans_symbol));
		}
		all_trans.push_back(std::move(trans_symbol));
		current_states = next_states;

		if (!anchor_end)
		{
			auto it = std::find_if (
				current_states.begin(),
				current_states.end(),
				[] (NFAState *st) { return st->is_end; }
			);
			if (it != current_states.end())
			{
				constructPath(all_trans, path, path_tags);
				return {true, buildAllCaptures(path, path_tags)};
			}
		}
	}

	auto it = std::find_if (
		current_states.begin(), current_states.end(),
		[] (NFAState *st) { return st->is_end; }
	);

	bool found = it != current_states.end();
	if (!found && !anchor_start && !str.empty())
	{
		auto next_match = matchAll(&str[0] + 1);
		return { found || next_match.first, next_match.second };
	}

	std::vector<std::string> caps;
	if (found)
	{
		constructPath(all_trans, path, path_tags);
		caps = buildAllCaptures(path, path_tags);
	}
	return {found, caps};
}

void	Regex::setNextStates(
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

void Regex::constructPath (
	const std::vector< std::vector<NFATransition> > & all_trans,
	std::vector<char>& path, std::vector<ft::set<int>>& path_tags
) const
{
	NFAState* last = automaton.end;

	for (auto rit = all_trans.rbegin(); rit != all_trans.rend(); ++rit)
	{
		auto& vec = *rit;
		auto trans = std::find_if(vec.begin(), vec.end(),
		[&] (const NFATransition& trans) {
			return trans.transition_state == last;
		});
		path_tags.push_back(trans->capture_tags);
		path.push_back(trans->symbol);

		last = trans->start_state;
	}
}

std::string Regex::buildCapture (
	const std::vector<char>& path, const std::vector<ft::set<int>>& tags, int capture
) const
{
	std::string cap;

	cap.reserve(path.size());

	auto path_it = path.rbegin();
	auto tags_it = tags.rbegin();
	for (; path_it != path.rend(); ++path_it, ++tags_it)
	{
		if (tags_it->count(capture))
			cap.append(1, *path_it);
	}

	return cap;
}

std::vector<std::string> Regex::buildAllCaptures (
	const std::vector<char>& path, const std::vector<ft::set<int>>& tags
) const
{
	std::vector<std::string> captures;
	captures.reserve(capture_index);
	for (int i = 0; i < capture_index; ++i)
		captures.push_back(buildCapture(path, tags, i));
	return captures;
}

NFA Regex::axiom()
{
	if (peek() == '^')
	{
		next();
		anchor_start = true;
	}
	NFA exp = expr(capture_index++);
	if (peek() == '$')
	{
		next();
		anchor_end = true;
	}
	return exp;
}

NFA Regex::expr(int capture)
{
	NFA trm = term();

	if (peek() == '|')
	{
		eat('|');
		NFA exp = expr(-1);
		if (capture == -1)
			return NFA::unify(trm, exp);
		else
			return NFA::capture(NFA::unify(trm, exp), capture);
	}

	// return trm;
	if (capture == -1)
		return trm;
	else
		return NFA::capture(trm, capture);
}

NFA Regex::term()
{
	NFA fact = factor();
	if (more())
	{
		if (peek() != '|' && peek() != ')' && peek() != '$')
			return NFA::concat(fact, term());
	}
	return fact;
}

NFA Regex::factor()
{
	NFA atm = atom();
	if (more() && std::string("?*+{").find(peek()) != std::string::npos)
	{
		return modifier(atm);
	}

	return atm;
}

NFA Regex::modifier(NFA atm)
{
	char c = next();
	switch (c)
	{
		case '*': return NFA::closure(atm);
		case '?': return NFA::questionmark(atm);
		case '+': return NFA::plus(atm);
		case '{': {
			NFA quant = quantify(atm);
			eat('}');
			return quant;
		}
		default: break;
	}
	
	return atm;
}

NFA Regex::quantify(NFA atm)
{
	std::string smin;
	while (std::isdigit(peek()))
		smin.append(1, next());
	int min = smin.empty() ? 0 : std::stoi(smin);
	if (peek() == '}')
		return NFA::quantify(min, min, atm);

	eat(',');

	std::string smax;
	while (std::isdigit(peek()))
		smax.append(1, next());
	int max = smax.empty() ? -1 : std::stoi(smax);

	if (max == -1)
		return NFA::quantify(min, -1, atm);
	else if (max < 0 || min < 0 || max < min)
		throw std::runtime_error("Regex invalid range");
	
	return NFA::quantify(min, max, atm);
}

NFA Regex::atom()
{
	if (peek() == '(')
	{
		eat('(');
		NFA exp;
		if (peek() == '?')
		{
			eat('?');
			eat(':');
			exp = expr(-1);
		}
		else 
			exp = expr(capture_index++);
		eat(')');
		return exp;
	}
	else if (peek() == '[')
	{
		eat('[');
		NFA br = bracket();
		eat(']');
		return br;
	}
	else
		return charset();
}

NFA	Regex::bracket()
{
	std::vector<char> set;
	set.reserve(127);

	if (peek() == '^')
		return (next(), setof(false, set));
	else
		return setof(true, set);
	
}

NFA Regex::setof(bool include, std::vector<char>& set)
{
	subsetof(set);
	if (peek() != ']')
		return setof(include, set);

	if (!include)
	{
		std::sort(set.begin(), set.end());
		std::vector<char> nonset;
		nonset.reserve(127);
		std::set_difference (
			alphabet.begin(), alphabet.end(),
			set.begin(), set.end(),
			std::back_inserter(nonset)
		);
		set = std::move(nonset);
	}

	return NFA::fromSymbolSet(set);
}

std::vector<char> escapedset(char c)
{
	std::vector<char> set;
	set.reserve(10);
	switch (c)
	{
		case 'd': {
			for (int i = 0; i < 10; ++i)
				set.push_back(i + '0');
			break;
		}
		case 's': {
			for (int i = 0; i < 5; ++i)
				set.push_back(i + 9);
			set.push_back(32);
			break;
		}
		default: {
			set.push_back(c);
			break;
		}
	}
	return set;
}

void Regex::subsetof(std::vector<char>& set)
{
	if (peek() == '\\')
	{
		next();
		std::vector<char> ret = escapedset(next());
		set.insert(set.end(), ret.begin(), ret.end());
		return;
	}
	char start = next();
	set.push_back(start);
	if (peek() == '-')
	{
		eat('-');
		char end = next();
		if (!more())
			throw std::runtime_error("Regex: Class is missing end bracket");
		if (end < start)
			throw std::runtime_error("Regex: Invalid ASCII range in class");
		for (char c = start + 1; c <= end; c++)
			set.push_back(c);
	}
}


NFA Regex::charset()
{
	if (peek() == '\\')
	{
		eat('\\');
		// any char
		if (peek() == 'd' || peek() == 's')
		{
			std::vector<char> set = escapedset(next());
			return NFA::fromSymbolSet(set);
		}
		return NFA::fromSymbol(next());
	}
	else
	{
		if (std::string("?*+").find(peek()) != std::string::npos)
			throw std::runtime_error("Invalid metachar token in Regex pattern");
		if (peek() == '.')
		{
			next();
			return NFA::fromSymbolSet(alphabet);
		}
		return NFA::fromSymbol(next());
	}
	
}

char Regex::peek()
{
	return pattern[index];
}

char Regex::next()
{
	return pattern[index++];
}

void Regex::eat(char token)
{
	if (peek() != token)
	{
		if (!peek())
			throw std::runtime_error("Regex: Unexpected end of pattern");
		std::string err = peek() ? std::string(1, peek()) : "(null)";
		err = "Regex: Invalid token `" + err + "`"; 
		throw std::runtime_error(err);
	}
	++index;
}

bool Regex::more()
{
	return index < pattern.size();
}
