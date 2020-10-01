#pragma once

#include <header.h>

#include <ByteBuffer.hpp>
#include <Regex.hpp>
#include <Utils.hpp>

class RequestBuffer
{
	private:
		std::string			current_headers;
		ssize_t				content_length;
		ByteBuffer			request_buffer;

	public:
		RequestBuffer();

		void				resetState();
		void				append(char *buf, size_t len);
		bool				ready() const;
		ByteBuffer			extract(bool remove = true);
		ssize_t				getContentLength(size_t header_break);
		const ByteBuffer&	buffer() const;

		static std::pair<std::string, std::string>
		extractHeader(const std::string& str);
};