#pragma once

#include "header.h"
#include <string>
#include <iostream>

class RequestParser {
	private:
		std::vector<std::string> _temp;
		std::vector<std::string> _headers;

		std::string _method;
		std::string _resource;
		std::string _protocol;
		std::string _accept_charset;
		std::string _accept_language;
		std::string _allow;
		std::string _authorization;
		std::string _content_language;
		std::string _content_length;
		std::string _content_location;
		std::string _content_type;
		std::string _date;
		std::string _host;
		std::string _referer;
		std::string _user_agent;

		void accept_charset_parser(std::vector<std::string> &head);
		void accept_language_parser(std::vector<std::string> &head);
		void allow_parser(std::vector<std::string> &head);
		void authorization_parser(std::vector<std::string> &head);
		void content_language_parser(std::vector<std::string> &head);
		void content_length_parser(std::vector<std::string> &head);
		void content_location_parser(std::vector<std::string> &head);
		void content_type_parser(std::vector<std::string> &head);
		void date_parser(std::vector<std::string> &head);
		void host_parser(std::vector<std::string> &head);
		void referer_parser(std::vector<std::string> &head);
		void user_agent_parser(std::vector<std::string> &head);

	public:
		RequestParser();
		~RequestParser();
		int parser(const std::string header);

		std::string getMethod() {return _method;}
		std::string getResource() {return _resource;}
		std::string getProtocol() {return _protocol;}


};