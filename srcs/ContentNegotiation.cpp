#include <ContentNegotiation.hpp>

Variant::Variant() {}

Variant::Variant(const std::string& type, const std::string& lang, const std::string& charset)
	: type(type), language(lang), charset(charset)
{
	
}

TypemapParser::TypemapParser()
	: token_index(0)
{
	
}

void	TypemapParser::typemap_error(const std::string& s)
{
	throw std::runtime_error(s + " (" + typemap_path + ")");
}

bool TypemapParser::is_header(const std::string& s)
{
	auto h = strtoupper(s);
	if (h != "CONTENT-TYPE" && h != "CONTENT-LANGUAGE")
		return false;
	return true;
}

std::vector<Variant> TypemapParser::parse(const std::string& typemap_path)
{
	this->typemap_path = typemap_path;

	std::string typemap = filetostr(typemap_path);
	tokens = tokenize(typemap);
	axiom();

	for (auto& v : variants)
	{
		if (v.type.empty())
			v.type = get_mime_type(v.uri);
		if (v.language.empty())
			v.language = "*";
	}
	return variants;
}

void TypemapParser::axiom()
{
	variant();
	if (more())
		axiom();
}

void TypemapParser::variant()
{
	Variant var;
	var.uri = peek();
	next();
	headers(var);
	variants.push_back(std::move(var));
}

void TypemapParser::headers(Variant& var)
{
	header(var);
	if (more() && is_header(peek()))
		headers(var);
}

void TypemapParser::header(Variant& var)
{
	auto key = peek();
	if (!is_header(key))
		typemap_error("unrecognized header in variant map");
	next();
	eat(":");
	if (strtoupper(key) == "CONTENT-TYPE")
		type_value(var);
	else if (strtoupper(key) == "CONTENT-LANGUAGE")
		lang_value(var);
}

void TypemapParser::type_value(Variant& var)
{
	var.type = peek();
	next();
	if (peek() == ";")
	{
		next();
		charset_value(var);
	}
}

void TypemapParser::lang_value(Variant& var)
{
	var.language = peek();
	next();
}

void TypemapParser::charset_value(Variant& var)
{
	eat("charset");
	eat("=");
	var.charset = peek();
	next();
}

void		TypemapParser::eat(const std::string& s)
{
	if (!more())
		typemap_error("unexpected end of file while parsing variant map");
	if (s != peek())
		typemap_error("bad token `" + peek() + "` in variant map");
	next();
}

std::string TypemapParser::peek()
{

	if (!more())
		return "";
	return tokens[token_index];
}

void 		TypemapParser::next()
{
	token_index++;
}

bool		TypemapParser::more()
{
	return token_index < tokens.size();
}


std::vector<std::string> TypemapParser::tokenize(std::string& tm_content)
{
	auto is_wp = [](char c){ return c == ' ' || c == '\t' || c == '\n'; };
	auto is_delim = [](char c){ return c == ':' || c == ';' || c == '='; };

	std::vector<std::string> tokens;
	std::string buf;
	tokens.reserve(30);

	for (auto& c : tm_content)
	{
		if (is_wp(c) || is_delim(c))
		{
			if (!buf.empty())
				tokens.push_back(buf);
			if (is_delim(c))
				tokens.push_back(std::string(1, c));
			buf.clear();
		}
		else
			buf.push_back(c);
	}
	if (!buf.empty())
		tokens.push_back(buf);

	return tokens;
}

ContentNegotiation::ContentNegotiation(RequestParser& req_parser)
	: req_parser(req_parser) {}

ssize_t		ContentNegotiation::get_pos_score(const std::string& s, const std::vector<std::string>& vec)
{
	if (s == "*" || s == "*/*")
		return vec.size();
	auto it = std::find(vec.begin(), vec.end(), s);
	if (it == vec.end())
		return -1;
	return std::distance(it, vec.end());
}

std::vector<double>		ContentNegotiation::score_variants(std::vector<Variant> variants)
{
	auto accept_charset = req_parser.getAcceptCharset();
	auto accept_language = req_parser.getAcceptLanguage();
	auto accept = req_parser.getAccept();

	double charset_mult = 3;
	double type_mult = 2;
	double lang_mult = 1.25;

	std::vector<double> scores;
	for (auto& variant : variants)
	{
		double score = 0;
		
		ssize_t type_pos = get_pos_score(variant.type, accept);
		ssize_t charset_pos = get_pos_score(variant.charset, accept_charset);
		ssize_t lang_pos = get_pos_score(variant.language, accept_language);
		if ((type_pos == -1 && !accept.empty()) ||
			(charset_pos == -1 && !accept_charset.empty()) ||
			(lang_pos == -1 && !accept_language.empty()))
		{
			scores.push_back(-1.0);
		}
		else
		{
			score += type_pos != -1 ? type_pos * type_mult : 0;
			score += charset_pos != -1 ? charset_pos * charset_mult : 0;
			score += lang_pos != -1 ? lang_pos * lang_mult : 0;
			scores.push_back(score);
		}
	}
	return scores;
}

Variant ContentNegotiation::negotiate(const std::vector<Variant>& variants)
{
	std::vector<double> scores = score_variants(variants);

	auto max_score = std::max_element(scores.begin(), scores.end());
	size_t chosen_index = std::distance(scores.begin(), max_score);
	
	if (scores.at(chosen_index) == -1)
		return {};
	else
		return variants.at(chosen_index);
}
