#pragma once

#include "header.h"
#include <string>
#include <iostream>
#include <RequestParser.hpp>
#include <Regex.hpp>
#include <Utils.hpp>
#include <ByteBuffer.hpp>
#include <RequestRouter.hpp>

class ResponseConstructor {
	private:
		ByteBuffer                 _header;

		std::string					_first_line;
		std::string					_code;
		int							_error;

		std::vector<std::string>	_allow;
		std::vector<std::string>	_content_language;
		std::string					_content_length;
		std::string					_content_location;
		std::string					_content_type;
		std::string					_date;
        std::string                 _last_modified;
        std::string                 _location;//3xx or 201
        std::string                 _retry_after;
        std::string                 _server;
        std::string                 _transfer_encoding;
        std::string                 _www_authenticate;

		std::string date();
		std::string retry_after();
		std::string www_authenticate(FileRequest file_request);
		std::string last_modified(FileRequest file_request);
		void		content_length(FileRequest file_request);
		std::string content_location(FileRequest file_request);
		std::string location(FileRequest file_request);
		std::string content_type(FileRequest file_request);

	public:
		ResponseConstructor();
		~ResponseConstructor();
		ByteBuffer constructor(RequestParser &req, FileRequest &file_request);
};