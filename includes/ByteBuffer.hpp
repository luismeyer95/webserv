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
		std::string 	str() const;
		void			advance(size_t num);

		ssize_t			find(const std::vector<BYTE>& needle) const;

		bool			empty() const;
		ByteBuffer		sub(size_t pos, size_t len = std::string::npos) const;

		void			append(const ByteBuffer& bb);
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

		friend std::ostream& operator<<(std::ostream& o, const ByteBuffer& bb)
		{
			for (auto& c : bb.data)
				o << c;
			return o;
		}
};