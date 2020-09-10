#pragma once

#include "header.h"
#include <string>
#include <iostream>
#include <RequestParser.hpp>

struct ContentType {
	std::string media_type;
	std::string subtype;
	std::string charset;
	std::string boundary;
};

class ResponseConstructor {
	private:
		std::string                 _header;

		std::string					_method;
		std::string					_code;

		std::vector<std::string>	_allow;
		std::vector<std::string>	_content_language;
		std::string					_content_length;
		std::string					_content_location;
		ContentType					_content_type;
		std::string					_date;
        std::string                 _last_modified;
        std::string                 _location;
        std::string                 _retry_after;
        std::string                 _server;
        std::string                 _transfer_encoding;
        std::string                 _www_authenticate;


	public:
		ResponseConstructor();
		~ResponseConstructor();
		void constructor(RequestParser req);
};