#pragma once

#include "header.h"
#include <string>
#include <iostream>
#include <Utils.hpp>
#include <ByteBuffer.hpp>

// struct ContentType {
// 	std::string media_type;
// 	std::string subtype;
// 	std::string charset;
// 	std::string boundary;
// };

// struct UserAgent {
// 	std::string product;
// 	std::string version;
// 	std::string comment;
// };

struct ContentType {
	std::string media_type;
	std::string subtype;
	std::string charset;
	std::string boundary;
};

struct Authorization {
	std::string method;
	std::string basic;

	std::string username;
	std::string realm;
	std::string domain;
	std::string nonce;
	std::string opaque;
	std::string algorithm;
	std::string uri;
	std::string qop;
	std::string response;
	std::string nc;
	std::string cnonce;
};

struct UserAgent {
	std::string product;
	std::string version;
	std::string comment;
};

class RequestParser {
	private:
		std::vector<std::string> _headers;

		int							_error;

		ByteBuffer					_payload;

		std::string					_method;
		std::string					_resource;
		std::string					_protocol;

		std::vector<std::string>	_accept_charset;
		std::vector<std::string>	_accept_language;
		std::vector<std::string>	_allow;
		std::string					_authorization;
		std::vector<std::string>	_content_language;
		size_t						_content_length;
		std::string					_content_location;
		ContentType					_content_type;
		std::string					_date;
		std::string					_host_name;
		unsigned short				_host_ip;
		std::string					_referer;
		UserAgent					_user_agent;

		std::string					_raw_accept_charset;
		std::string					_raw_accept_language;
		std::string					_raw_content_language;
		std::string					_raw_content_type;
		std::string					_raw_user_agent;

		std::vector<std::string>	_req_methods;

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
		int parser(const ByteBuffer request);

		std::string					getMethod() {return _method;}
		std::string&				getResource() {return _resource;}
		std::string					getProtocol() {return _protocol;}
		std::vector<std::string>	getAcceptCharset() {return _accept_charset;}
		std::vector<std::string>	getAcceptLanguage() {return _accept_language;}
		std::vector<std::string>	getAllow() {return _allow;}
		std::string					getAuthorization() {return _authorization;}
		std::vector<std::string>	getContentLanguage() {return _content_language;}
		size_t						getContentLength() {return _content_length;}
		std::string					getContentLocation() {return _content_location;}
		ContentType					getContentType() {return _content_type;}
		std::string					getDate() {return _date;}
		std::string					getHost() {return _host_name;}
		unsigned short				getIpHost() {return _host_ip;}
		std::string					getReferer() {return _referer;}
		UserAgent					getUserAgent() {return _user_agent;}
		int							getError() {return _error;}
		ByteBuffer&					getPayload() {return _payload;}
		std::string					getRawAcceptCharset() {return _raw_accept_charset;}
		std::string					getRawAcceptLanguage() {return _raw_accept_language;}
		std::string					getRawContentLanguage() {return _raw_content_language;}
		std::string					getRawContentType() {return _raw_content_type;}
		std::string					getRawUserAgent() {return _raw_user_agent;}

};