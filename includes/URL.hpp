#pragma once

#include "header.h"
#include "Regex.hpp"
#include <list>

// Used to generate helper objects for splitting URLs into components
class Tokenizer
{
	private:
		int head;

		std::list<std::string> delim_set;
		std::list<std::string> skip_set;

		std::pair<size_t, size_t> find_first_of_str
			(const std::string& str, const std::list<std::string>& strs);
	public:
		Tokenizer
			(const std::list<std::string>& delim_set, const std::list<std::string>& skip_set);

		std::string get_token(std::string& str);
		void		reset();
};

class URL
{
	private:
		std::string _encoded_url;

		std::string _scheme;
		std::string _host;
		std::string	_hosttype;
		std::string _port;
		std::string _path;
		std::string _query;
		std::string _fragment;

		bool				check (
			const std::string& rgx, const std::string& str, bool thrw, const std::string& error
		);

	public:

		enum class Component {
			Scheme, Host, Port, Path, Query, Fragment
		};

		URL();
		URL(const URL& o);
		URL& operator=(const URL& o);
		URL (
			const std::string& scheme,
			const std::string& host,
			const std::string& port,
			const std::string& path,
			const std::string& query,
			const std::string& fragment
		);

		URL(const std::string& encoded_url);

		std::string&		get(URL::Component comp);
		std::string			getFullURL();

		// from UTF-8 to percent-encoding
		static std::string	encode(URL::Component comp, const std::string& str);
		static std::string	encode(const Regex& rgx, const std::string& str);
		static std::string	encode(const std::string& str);

		static std::string	decode(const std::string& str);

		static std::string	reformatPath(std::string input_url);

		bool				isPartialURI();


		void				validate(URL::Component comp);
		void				validateAllComponents();

		void				printComponents();
		void				printDecoded();

		// bool				operator==(const URL& o);
};