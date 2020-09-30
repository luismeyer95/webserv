#include <ByteBuffer.hpp>

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

ssize_t		ByteBuffer::find(const std::vector<BYTE>& needle) const
{
	if (needle.empty())
		return (0);
	auto it = std::search(data.begin(), data.end(), needle.begin(), needle.end());
	if (it == data.end())
		return -1;
	return std::distance(data.begin(), it);
}

bool			ByteBuffer::empty() const
{
	return size() == 0;
}

ByteBuffer			ByteBuffer::sub(size_t pos, size_t len) const
{
	if (len == std::string::npos)
		len = size();
	ByteBuffer ret;
	ret.append(get() + pos, len);
	return ret;
}

std::string 	ByteBuffer::str() const
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

void	ByteBuffer::append(const ByteBuffer& bb)
{
	append(bb.get(), bb.size());
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
	// check if it is a valid file
	struct stat buffer;
	if (stat(filename.c_str(), &buffer) != 0 || !(buffer.st_mode & S_IFREG))
		throw std::runtime_error("ByteBuffer::peekFileSize() : not a file");

	// open the file:
	std::streampos file_size;
	std::ifstream file(filename.c_str(), std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("ByteBuffer::peekFileSize() : error opening file");

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
