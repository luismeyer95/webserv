#pragma once

#include "header.h"
typedef unsigned char BYTE;

class ByteBuffer
{
	private:
		std::vector<BYTE>	data;
		size_t				head;
	public:

		ByteBuffer()
			: head(0) {}
		~ByteBuffer() {}

		ByteBuffer(const ByteBuffer& o)
		{
			*this = o;
		}

		ByteBuffer&	operator=(const ByteBuffer& o)
		{
			data = o.data;
			head = o.head;
			return *this;
		}

		void	appendBuffer(const BYTE* buffer, size_t size)
		{
			data.insert(data.end(), buffer, buffer + size);
		}

		void	appendBinaryData(const std::string& filename)
		{
			// open the file:
			std::streampos file_size;
			std::ifstream file(filename.c_str(), std::ios::binary);

			// get its size:
			file.seekg(0, std::ios::end);
			file_size = file.tellg();
			file.seekg(0, std::ios::beg);

			// read the data:
			size_t cur_size = data.size();
			data.resize(cur_size + (size_t)file_size);
			file.read((char*)&data[cur_size], file_size);

			file.close();
		}

		size_t	peekSize(const std::string& filename)
		{
			// open the file:
			std::cout << "opening file " << filename << std::endl;
			std::streampos file_size;
			std::ifstream file(filename.c_str(), std::ios::binary);
			if (file.is_open())
				std::cout << "successful open" << std::endl;

			// get its size:
			file.seekg(0, std::ios::end);
			file_size = file.tellg();
			file.seekg(0, std::ios::beg);

			file.close();
			return (size_t)file_size;
		}

		const BYTE* get() const { return &data[0] + head; }

		void	advance(size_t num)
		{
			head += num;
		}

		size_t	size() const
		{
			return data.size() - head;
		}

		ByteBuffer	operator+(const ByteBuffer& o)
		{
			ByteBuffer ret(*this);
			ret.appendBuffer(o.get(), o.size());
			return ret;
		}

		ByteBuffer&	operator+=(const ByteBuffer& o)
		{
			return (*this = *this + o);
		}

		std::string 	str()
		{
			return std::string((char*)get(), size());
		}
};