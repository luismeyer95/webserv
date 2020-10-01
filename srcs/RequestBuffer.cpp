#include <RequestBuffer.hpp>

RequestBuffer::RequestBuffer() {}

void	RequestBuffer::append(char *buf, size_t len)
{
	request_buffer.append((BYTE*)buf, len);
	if (current_headers.empty())
		resetState();
}

void RequestBuffer::resetState()
{
	ssize_t header_break = request_buffer.strfind("\r\n\r\n");
	if (header_break != -1)
		content_length = getContentLength(header_break + 4);
}

const ByteBuffer& RequestBuffer::buffer() const
{
	return request_buffer;
}

bool	RequestBuffer::ready() const
{
	if (content_length == -1)
		return false;
	size_t total_needed = current_headers.size() + static_cast<size_t>(content_length);
	return request_buffer.size() >= total_needed;
}

ByteBuffer RequestBuffer::extract(bool remove)
{
	ByteBuffer request;

	size_t total_needed = current_headers.size() + static_cast<size_t>(content_length);
	request.append((BYTE*)request_buffer.get(), total_needed);

	if (remove)
	{
		request_buffer = request_buffer.sub(total_needed);
		current_headers.clear();
		content_length = -1;
		resetState();
	}

	return request;
}

ssize_t RequestBuffer::getContentLength(size_t header_break)
{
	current_headers = request_buffer.sub(0, header_break).str();
	auto vec_headers = strsplit(current_headers, "\r\n");
	for (auto& s : vec_headers)
	{
		auto keyval = extractHeader(s);
		if (keyval.first == "Content-Length")
		{
			try { return std::stoull(keyval.second); }
			catch(...) {} 
		}
	}
	return 0;
}

std::pair<std::string, std::string> RequestBuffer::extractHeader(const std::string& str)
{
	static Regex rgx("^([^\\s]+): *([^\\s]+.*)$");

	auto res = rgx.matchAll(str);
	if (!res.first)
		return {};
	std::string key = res.second.at(1);
	std::string value = res.second.at(2);
	return {key, value};
}