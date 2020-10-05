#pragma once

#include <header.h>
#include <ByteBuffer.hpp>
#include <ErrorCode.hpp>
#include <Logger.hpp>

const size_t MAXBUF = 8192;
const int TIMEOUT = 4000000;

class ResponseBuffer
{
	protected:
		using string = std::string;
		typedef std::vector<string> string_vec;
		typedef std::vector<char*> cstring_vec;
		typedef int buf_type;
		typedef int fd_type;

		string_vec			cgi_headers_buf;
		ByteBuffer			buffer;
	public:
		ResponseBuffer();

		virtual ByteBuffer& get(size_t bytes = MAXBUF);
		virtual void 		advance(size_t bytes);
		virtual bool 		eof();
		virtual string_vec& getHeaders();

		virtual ~ResponseBuffer();
};

class ResponseBufferFileStream : public ResponseBuffer
{
	protected:
		fd_type		in;
		string		file_path;
		bool		eof_flag;
		bool		chunked_flag;

	public:
		ResponseBufferFileStream(const string& filename, bool chunked);

		ByteBuffer& get(size_t bytes = MAXBUF);
		bool 		eof();
		void		advance(size_t bytes);
		void		readStream(size_t bytes);
		void		scriptError(const std::string& errlog, bool thrw = true);

		~ResponseBufferFileStream();
};


class ResponseBufferProcessStream : public ResponseBuffer
{
	private:
		fd_type		pip_in[2];
		fd_type		pip_out[2];
		int			pstatus;
		pid_t		worker_pid;
		pid_t		timer_pid;

		bool		eof_flag;
		bool		chunked_flag;
		string		file_path;

	public:
		
		ByteBuffer& get(size_t bytes = MAXBUF);
		void		readStream(size_t bytes);
		void		advance(size_t bytes);
		bool		eof();

		void		scriptError(const std::string& errlog, bool thrw = true);

		// Constructs a CGI process-output stream buffer
		ResponseBufferProcessStream (
			const string& script_path, const ByteBuffer& request_payload,
			const string& bin, const cstring_vec& argv, const cstring_vec& env, bool chunked);

		~ResponseBufferProcessStream();
		void		execScript(const string& bin, const cstring_vec& argv, const cstring_vec& env);
		void		feedRequestPayload(ByteBuffer request_payload);
		bool		searchHeaderBreak(ssize_t& break_pos, ssize_t& break_len);
		void		storeHeaders();
		string_vec& getHeaders();
		void 		reap();

};