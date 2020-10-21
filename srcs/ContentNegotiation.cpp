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
	// for (auto& str : tokens)
	// 	http_print(str);
	axiom();
	// for (auto& e : variants)
	// {
	// 	std::cout << e.uri << std::endl;
	// 	std::cout << "type: " << e.type << std::endl;
	// 	std::cout << "lang: " << e.language << std::endl;
	// 	std::cout << "charset: " << e.charset << std::endl;
	// }

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
		throw std::runtime_error("unrecognized header in typemap");
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
		throw std::runtime_error("unexpected end of file while parsing typemap (" + typemap_path + ")");
	if (s != peek())
		throw std::runtime_error("bad token `" + peek() + "` in typemap (" + typemap_path + ")");
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

std::string ContentNegotiation::negotiate(const std::vector<Variant>& variants)
{
	auto accept_charset = req_parser.getAcceptCharset();
	auto accept_language = req_parser.getAcceptLanguage();
	auto accept = req_parser.getAccept();
}
