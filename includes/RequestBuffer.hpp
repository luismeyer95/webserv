#pragma once

#include <header.h>

#include <ByteBuffer.hpp>
#include <Regex.hpp>
#include <Utils.hpp>
#include <RequestParser.hpp>
#include <URL.hpp>
#include <RequestRouter.hpp>
#include <ResponseConstructor.hpp>

struct ClientSocket;
struct HTTPExchange;

class RequestBuffer
{
	private:
		RequestParser		req_parser;
		RequestRouter&		route;

		ClientSocket*		socket;
		
		size_t				max_body;
		ssize_t				header_break;
		size_t				content_length;
		int					errcode;
		bool				processed;

		bool				chunked_flag;
		bool				chunk_eof;
		size_t				chunked_body_len;
		ByteBuffer			chunk_buffer;

		ByteBuffer			request_buffer;

		template <typename T>
		bool				isSet(T var);
		ssize_t				headerBreak(ByteBuffer& bb);
		size_t				neededLength();
		size_t				maxRequestLength();

		bool				headerSizeError();
		bool				parserError();
		bool				processError(bool expr, int code);

		void				splitHeaderPayload();

		ssize_t				processChunk();
		void				dechunk();

		void				readHeader(char *buf, size_t len);
		void				readPayload(char *buf, size_t len);
		bool				processHeader();
		void				processRequest();
		void				processRequestIfPossible();

	public:
		RequestBuffer(RequestRouter& route, ClientSocket* sock);

		void				append(char *buf, size_t len);
		bool				ready() const;

		const ByteBuffer&	get() const;
};