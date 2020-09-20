#pragma once

#include "header.h"
typedef unsigned char BYTE;

class ByteBuffer
{
	private:
		std::vector<BYTE>	data;
		size_t				head;
	public:

		ByteBuffer();
		~ByteBuffer();
		ByteBuffer(const ByteBuffer& o);
		ByteBuffer&	operator=(const ByteBuffer& o);
		ByteBuffer&	operator=(const std::string& o);

		size_t			size() const;
		const BYTE*		get() const;
		std::string 	str();
		void			advance(size_t num);

		void			append(const BYTE* buffer, size_t size);
		void			append(const std::ostringstream& stream);

		static size_t 	peekFileSize(const std::string& filename);
		void			appendFile(const std::string& filename);

		ByteBuffer		operator+(const ByteBuffer& o);
		ByteBuffer&		operator+=(const ByteBuffer& o);

		template <typename T>
		friend ByteBuffer& operator<<(ByteBuffer& buf, T elem)
		{
			std::ostringstream stream;
			stream << elem;
			buf.append(stream);
			return buf;
		}
};