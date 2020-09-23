#pragma once

#include "header.h"
#include <string>
#include <iostream>
#include <RequestParser.hpp>
#include <Regex.hpp>
#include <Utils.hpp>
#include <ByteBuffer.hpp>
#include <RequestRouter.hpp>


struct ContentType {
	std::string media_type;
	std::string subtype;
	std::string charset;
	std::string boundary;
};

class ResponseConstructor {
	private:
		ByteBuffer                 _header;

		std::string					_first_line;
		std::string					_code;

		std::vector<std::string>	_allow;
		std::vector<std::string>	_content_language;
		std::string					_content_length;
		std::string					_content_location;
		ContentType					_content_type;
		std::string					_date;
        std::string                 _last_modified;
        std::string                 _location;//3xx or 201
        std::string                 _retry_after;
        std::string                 _server;
        std::string                 _transfer_encoding;
        std::string                 _www_authenticate;

		void date();
		void retry_after();
		void www_authenticate(FileRequest file_request);
		void last_modified(FileRequest file_request);

	public:
		ResponseConstructor();
		~ResponseConstructor();
		void constructor(RequestParser req);
};