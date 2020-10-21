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

		void						typemap_error(const std::string& s);

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

		ssize_t					get_pos_score(const std::string& s, const std::vector<std::string>& vec);
		std::vector<double>		score_variants(std::vector<Variant> variants);
	public:
		ContentNegotiation(RequestParser& req_parser);
		Variant					negotiate(const std::vector<Variant>& variants);
};