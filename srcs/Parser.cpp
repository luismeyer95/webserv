#include "../includes/Parser.hpp"

Parser& Parser::getInstance()
{
	static Parser parse;
	return parse;
}

