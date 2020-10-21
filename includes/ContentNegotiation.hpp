#pragma once

#include <header.h>
#include <Utils.hpp>
#include <RequestParser.hpp>

class Variant
{
	public:
		std::string	uri;
		std::string type;
		std::string language;
		std::string charset;
		Variant(const std::string& type, const std::string& lang, const std::string& charset);
		Variant();
};

class TypemapParser
{
	private:
		std::vector<Variant>		variants;
		std::string					typemap_path;

		std::vector<std::string> 	tokens;
		size_t 						token_index;

		std::vector<std::string>	tokenize(std::string& tm_content);

		bool is_header(const std::string& s);
		void axiom();
		void variant();
		void headers(Variant& var);
		void header(Variant& var);
		void type_value(Variant& var);
		void lang_value(Variant& var);
		void charset_value(Variant& var);

		void		eat(const std::string& s);
		std::string peek();
		void 		next();
		bool 		more();

	public:
		TypemapParser();
		std::vector<Variant> parse(const std::string& typemap_path);

};

class ContentNegotiation
{
	private:
		RequestParser&			req_parser;
	public:
		ContentNegotiation(RequestParser& req_parser);
		std::string				negotiate(const std::vector<Variant>& variants);
};