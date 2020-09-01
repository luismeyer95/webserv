#include "../includes/ByteBuffer.hpp"

ByteBuffer::ByteBuffer() : head(0) {}

ByteBuffer::~ByteBuffer() {}

ByteBuffer&	ByteBuffer::operator=(const ByteBuffer& o)
{
	data = o.data;
	head = o.head;
	return *this;
}

ByteBuffer::ByteBuffer(const ByteBuffer& o)
{
	*this = o;
}

size_t	ByteBuffer::size() const
{
	return data.size() - head;
}

const BYTE* ByteBuffer::get() const
{
	return &data[0] + head;
}

std::string 	ByteBuffer::str()
{
	return std::string((char*)get(), size());
}

void	ByteBuffer::advance(size_t num)
{
	head += num;
}

void	ByteBuffer::append(const BYTE* buffer, size_t size)
{
	data.insert(data.end(), buffer, buffer + size);
}

void	ByteBuffer::append(const std::ostringstream& stream)
{
	append((BYTE*)stream.str().c_str(), stream.str().size());
}

void	ByteBuffer::appendFile(const std::string& filename)
{
	size_t file_size = peekFileSize(filename);

	std::ifstream file(filename.c_str(), std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("Error opening file in ByteBuffer::appendFile()");

	// read the data:
	size_t cur_size = data.size();
	data.resize(cur_size + file_size);
	file.read((char*)&data[cur_size], file_size);

	file.close();
}

size_t	ByteBuffer::peekFileSize(const std::string& filename)
{
	// open the file:
	std::streampos file_size;
	std::ifstream file(filename.c_str(), std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("Error opening file in ByteBuffer::peekFileSize()");

	// get its size:
	file.seekg(0, std::ios::end);
	file_size = file.tellg();
	file.seekg(0, std::ios::beg);

	file.close();
	return (size_t)file_size;
}

ByteBuffer	ByteBuffer::operator+(const ByteBuffer& o)
{
	ByteBuffer ret(*this);
	ret.append(o.get(), o.size());
	return ret;
}

ByteBuffer&	ByteBuffer::operator+=(const ByteBuffer& o)
{
	return (*this = *this + o);
}
