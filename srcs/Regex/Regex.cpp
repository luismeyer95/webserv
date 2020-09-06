#include <Regex/Regex.hpp>

Regex::Regex(const std::string& pattern)
	: pattern(pattern), index(0),
	anchor_start(false), anchor_end(false), automaton()
{
	PatternValidation check(pattern);
	
	alphabet.reserve(127);
	for (auto i = 0; i < 127; ++i)
		alphabet.push_back((char)i);

	automaton = axiom();
}

Regex::~Regex()
{
	NFA::deleteAutomaton(automaton);
}

std::pair<bool, std::string> Regex::match(const std::string& str)
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
		{
			NFAState *next = state->transit(c);
			if (next)
			{
				setNextStates(next, next_states, visited);
				visited.clear();
			}
		}
		current_states = next_states;

		if (!anchor_end)
		{
			// if end of string is not anchored, return true
			// as soon as an accepting state is found

			auto it = std::find_if (
				current_states.begin(),
				current_states.end(),
				[] (NFAState *st) { return st->is_end; }
			);
			if (it != current_states.end())
			{
				// dirty way of getting the length of the matched string
				size_t len = (&c - &str[0]) + 1;
				return {true, str.substr(0, len)};
			}
		}
	}

	auto it = std::find_if (
		current_states.begin(),
		current_states.end(),
		[] (NFAState *st) { return st->is_end; }
	);

	bool found = it != current_states.end();

	// if no start anchor set, try to match again for next char until
	// end of string
	if (!anchor_start && !str.empty())
	{
		std::pair<bool, std::string> next_match = match(&str[0] + 1);
		return {
			found || next_match.first,
			next_match.second
		};
	}

	return {found, str};
}

void	Regex::setNextStates(
	NFAState *state,
	std::vector<NFAState*>& next_states,
	std::vector<NFAState*>& visited)
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

NFA Regex::axiom()
{
	if (peek() == '^')
	{
		next();
		anchor_start = true;
	}
	NFA exp = expr();
	if (peek() == '$')
	{
		next();
		anchor_end = true;
	}
	return exp;
}

NFA Regex::expr()
{
	NFA trm = term();

	if (peek() == '|')
	{
		eat('|');
		NFA exp = expr();
		return NFA::unify(trm, exp);
	}

	return trm;
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
		NFA exp = expr();
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

void Regex::subsetof(std::vector<char>& set)
{
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
		for (char c = start; c <= end; c++)
			set.push_back(c);
	}
}

NFA Regex::charset()
{
	if (peek() == '\\')
	{
		eat('\\');
		// any char
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
