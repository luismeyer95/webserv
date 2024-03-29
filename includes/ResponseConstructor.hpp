#pragma once

#include "header.h"
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

		std::string					_allow;//405
		std::string					_content_language;
		std::string					_content_length;//be careful lot of condition
		std::string					_content_location;
		std::string					_content_type;
		std::string					_date;
        std::string                 _last_modified;
        std::string                 _location;//3xx or 201
        std::string                 _retry_after;
        std::string                 _server;
        std::string                 _transfer_encoding;//304, (HEAD ?), NO 1xx, NO 204, NO Connect 2xx,
        std::string                 _www_authenticate;

		void date();
		void retry_after();
		void www_authenticate(FileRequest& file_request);
		void last_modified(FileRequest& file_request);
		void content_length(FileRequest& file_request);
		void content_language(FileRequest& file_request);
		void content_location(FileRequest& file_request);
		void location(FileRequest& file_request);
		void content_type(FileRequest& file_request);
		void allow(FileRequest& file_request);

	public:
		ResponseConstructor();
		~ResponseConstructor();
		ByteBuffer constructor(RequestParser &req, FileRequest &file_request);
};