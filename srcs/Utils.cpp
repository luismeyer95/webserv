#include <Utils.hpp>

std::string filetostr(std::string filename)
{
	int fd = open(filename.c_str(), O_RDONLY, 0644);
	if (fd == -1)
		throw std::runtime_error("could not open file");

	char buf[8192 + 1];
	ssize_t ret = 0;
	std::string out;
	
	while ( (ret = read(fd, buf, 8192)) > 0 )
	{
		buf[ret] = 0;
		out += buf;
	}
	close(fd);
	if (ret == -1)
		throw std::runtime_error("read(): " + std::string(strerror(errno)));
	return out;
}

std::vector<std::string> strsplit(const std::string& str, const std::string& delim_set)
{
	std::vector<std::string> tokens;
	size_t index = 0;
	size_t len = 0;

	for (size_t i = 0; i < str.size(); ++i)
	{
		if (delim_set.find(str[i]) != std::string::npos)
		{
			if (len)
				tokens.push_back(str.substr(index, len));
			index = i + 1;
			len = 0;
		}
		else
			len++;
	}
	if (len)
		tokens.push_back(str.substr(index, len));
	
	return tokens;
}

std::vector<BYTE> readbin(const std::string& filename)
{
    // open the file:
    std::streampos fileSize;
    std::ifstream file(filename.c_str(), std::ios::binary);

    // get its size:
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // read the data:
    std::vector<BYTE> fileData(fileSize);
    file.read((char*)&fileData[0], fileSize);
    return fileData;
}



std::string make_html_error_page(int error_code, const std::string& error_string)
{
	std::string page = "<div><div style=\"text-align: center;\"><h1>Error "
					+ std::to_string(error_code) + "</h1><p>"
					+ error_string + "</p></div></div>";
	return page;
}

void	http_print(const std::string& s)
{
	for (size_t i = 0; i < s.size(); ++i)
	{
		if (s[i] == '\n')
			std::cout << "\\n";
		else if (s[i] == '\r')
			std::cout << "\\r";
		else
			std::cout << s[i];
	}
	std::cout << std::endl;
}

void	dec_print(const char *s)
{
	for (size_t i = 0; s[i]; ++i)
		std::cout << (int)s[i] << " ";
	std::cout << std::endl;
}


std::string		get_gmt_time(time_t date)
{
	struct tm *info;
	char buffer[128];

	info = gmtime(&date);
	// Wed, 21 Oct 2015 07:28:00 GMT
	strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S %Z", info);
	return std::string(buffer);
}

bool is_number(const std::string s)
{
	std::string::const_iterator it = s.begin();
	while (it != s.end() && *it == ' ')
		it++;
	while (it != s.end() && std::isdigit(*it))
		it++;
	return (it == s.end());
}

std::string trim(const std::string &str)
{
	size_t first = str.find_first_not_of(' ');
	if (std::string::npos == first)
		return (str);
	size_t last = str.find_last_not_of(' ');
	return str.substr(first, (last - first + 1));
} 

std::vector<std::string> get_header_name(std::string str, char c)
{
	std::vector<std::string> out;
	std::stringstream   stream(str);
	std::string         temp;
	
	getline(stream, temp, c);
	out.push_back(temp);
	getline(stream, temp, '\r');
	out.push_back(trim(temp));
	/*
	out.push_back(str.substr(0, str.find(c)));
	out.push_back(str.substr(0, str.find('\n')));*/
	return (out);
}

std::vector<std::string> header_finder(const std::vector<std::string> lines, const std::string to_find)
{
	std::vector<std::string> out;
	
	for (int i = 0; i < (int)lines.size(); i++)
    {
        out = get_header_name(lines[i], ':');
        if (out[0] == to_find && out.size() == 2)
            break;
        if (i == (int)lines.size() - 1)
        {
            out.clear();
			out.resize(0);
            return (out);
        }
    }
    return (out);
}

bool check_str_len(const std::string &str, unsigned int min, unsigned int max)
{
	if (str.length() < min || str.length() > max)
		return (0);
	return (1);
}

bool is_http_date(std::string str)
{
	std::vector<std::string> tmp;
	std::string day_name[] = {"Mon,","Tue,", "Wed,","Thu,", "Fri,", "Sat,", "Sun,"};
	std::string month_name[] = {"Jan","Feb", "Mar","Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	// tmp = tokenizer(str, ',');
	tmp = strsplit(str, ",");
	if (tmp.size() != 6)
		return (0);
	for (int i = 0; i < 7; i++)
	{
		if (tmp[0] == day_name[i])
			break;
		if (i == 6)
			return (0);
	}
	if(tmp[1].length() != 2 || !is_number(tmp[1]) || tmp[3].length() != 4 || !is_number(tmp[3]))
		return (0);
	for (int i = 0; i < 12; i++)
	{
		if (tmp[2] == month_name[i])
			break;
		if (i == 12)
			return (0);
	}
	if (tmp[5] != "GMT")
		return (0);
	// tmp = tokenizer(tmp[4], ':');
	tmp = strsplit(tmp[4], ":");
	if (tmp.size() != 3 || tmp[0].length() != 2 || !is_number(tmp[0]) ||tmp[1].length() != 2 || !is_number(tmp[1]) ||tmp[2].length() != 2 || !is_number(tmp[2]))
		return (0);
	return (1);
}

std::string get_current_dir()
{
	char buf[2048];

	if (!getcwd(buf, sizeof(buf)))
		return "";
	return std::string(buf);
}

std::string get_http_string(int code)
{
	static std::map< int, std::string > codemap
	({
		{100, "Continue"},
		{101, "Switching Protocols"},
		{200, "OK"},
		{201, "Created"},
		{202, "Accepted"},
		{203, "Non-Authoritative Information"},
		{204, "No Content"},
		{205, "Reset Content"},
		{206, "Partial Content"},
		{300, "Multiple Choices"},
		{301, "Moved Permanently"},
		{302, "Found"},
		{303, "See Other"},
		{304, "Not Modified"},
		{305, "Use Proxy"},
		{307, "Temporary Redirect"},
		{400, "Bad Request"},
		{401, "Unauthorized"},
		{402, "Payment Required"},
		{403, "Forbidden"},
		{404, "Not Found"},
		{405, "Method Not Allowed"},
		{406, "Not Aceptable"},
		{407, "Proxy Authentication Required"},
		{408, "Request Time-out"},
		{409, "Conflict"},
		{410, "Gone"},
		{411, "Length Required"},
		{412, "Precondition Failed"},
		{413, "Payload Too Large"},
		{414, "Request-URI Too Large"},
		{415, "Unsupported Media Type"},
		{416, "Requested range not satisfiable"},
		{417, "Expectation Failed"},
		{429, "Too Many Requests"},
		{431, "Request Header Fields Too Large"},
		{500, "Internal Server Error"},
		{501, "Not Implemented"},
		{502, "Bad Gateway"},
		{503, "Service Unavailable"},
		{504, "Gateway Time-out"},
		{505, "HTTP Version not supported"}
	});

	if (codemap.count(code))
		return codemap.at(code);
	return "Bad Code";
}

std::string					get_mime_type(const std::string& path)
{
	auto split_segment = strsplit(URL::reformatPath(path), "/");
	std::string last_segment = split_segment.empty() ? "" : split_segment.back();

	if (last_segment.find('.') == std::string::npos)
		return "text/plain";
	std::string ext = strsplit(last_segment, ".").back();
	auto search = [&ext] (const std::pair<const std::string, std::vector<std::string> >& e)
	{
		return std::find(e.second.begin(), e.second.end(), ext) != e.second.end();
	};
	auto& mimemap = mime_types();
	auto find = std::find_if(mimemap.begin(), mimemap.end(), search);
	if (find == mimemap.end())
		return "text/plain";
	return find->first;
}

std::map<std::string, std::vector<std::string>>& mime_types()
{
	static std::map< std::string, std::vector<std::string> > mimemap
	({
		{"text/html", {"html", "htm", "shtml"}},
		{"text/css", {"css"}},
		{"audio/aac", {"aac"}},
		{"application/x-abiword", {"abw"}},
		{"application/x-freearc", {"arc"}},
		{"video/x-msvideo", {"avi"}},
		{"application/vnd.amazon.ebook", {"azw"}},
		{"application/octet-stream", {"bin", "exe", "deb", "dll", "dmg", "iso", "img"}},
		{"image/bmp", {"bmp"}},
		{"application/x-bzip", {"bz"}},
		{"application/x-bzip2", {"bz2"}},
		{"application/x-csh", {"csh"}},
		{"text/csv", {"csv"}},
		{"application/msword", {"doc"}},
		{"application/vnd.openxmlformats-officedocument.wordprocessingml.document", {"docx"}},
		{"application/vnd.ms-fontobject", {"eot"}},
		{"application/epub+zip", {"epub"}},
		{"application/gzip", {"gz"}},
		{"image/gif", {"gif"}},
		{"image/vnd.microsoft.icon", {"ico"}},
		{"text/calendar", {"ics"}},
		{"application/java-archive", {"jar"}},
		{"image/jpeg", {"jpeg", "jpg"}},
		{"text/javascript", {"js", "mjs"}},
		{"application/json", {"json"}},
		{"application/ld+json", {"jsonld"}},
		{"audio/midi", {"midi", "mid"}},
		{"audio/mpeg", {"mp3"}},
		{"video/mpeg", {"mpeg"}},
		{"application/vnd.apple.installer+xml", {"mpkg"}},
		{"application/vnd.oasis.opendocument.presentation", {"odp"}},
		{"application/vnd.oasis.opendocument.spreadsheet", {"ods"}},
		{"application/vnd.oasis.opendocument.text", {"odt"}},
		{"audio/ogg", {"oga"}},
		{"video/ogg", {"ogv"}},
		{"application/ogg", {"ogx"}},
		{"audio/opus", {"opus"}},
		{"font/otf", {"otf"}},
		{"image/png", {"png"}},
		{"application/pdf", {"pdf"}},
		{"application/x-httpd-php", {"php"}},
		{"application/vnd.ms-powerpoint", {"ppt"}},
		{"application/vnd.openxmlformats-officedocument.presentationml.presentation", {"pptx"}},
		{"application/vnd.rar", {"rar"}},
		{"application/rtf", {"rtf"}},
		{"application/x-sh", {"sh"}},
		{"image/svg+xml", {"svg"}},
		{"application/x-shockwave-flash", {"swf"}},
		{"application/x-tar", {"tar"}},
		{"image/tiff", {"tiff", "tif"}},
		{"video/mp2t", {"ts"}},
		{"font/ttf", {"ttf"}},
		{"text/plain", {"txt"}},
		{"application/vnd.visio", {"vsd"}},
		{"audio/wav", {"wav"}},
		{"audio/webm", {"weba"}},
		{"video/webm", {"webm"}},
		{"image/webp", {"webp"}},
		{"font/woff", {"woff"}},
		{"font/woff2", {"woff2"}},
		{"application/xhtml+xml", {"xhtml"}},
		{"application/vnd.ms-excel", {"xls"}},
		{"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet", {"xlsx"}},
		{"application/xml", {"xml"}},
		{"application/vnd.mozilla.xul+xml", {"xul"}},
		{"application/zip", {"zip"}},
		{"video/3gpp", {"3gp"}},
		{"video/3gpp2", {"3g2"}},
		{"application/x-7z-compressed", {"7z"}}
	});

	return mimemap;
}

std::string ntohexstr(size_t num)
{
    static const char* digits = "0123456789ABCDEF";
    std::string rc(16, '0');
    for (size_t i = 0, j = (16 - 1) * 4 ; i < 16; ++i, j -= 4)
        rc[i] = digits[(num >> j) & 0x0f];
	size_t z = 0;
	for (; rc[z] == '0'; ++z);
	if (z == rc.size())
		z = rc.size() - 1;
	return rc.substr(z);
}

size_t	peek_file_size(const std::string& filename)
{
	// check if it is a valid file
	struct stat buffer;
	if (stat(filename.c_str(), &buffer) != 0 || !(buffer.st_mode & S_IFREG))
		throw std::runtime_error("peek_file_size(): not a file");

	// open the file:
	std::streampos file_size;
	std::ifstream file(filename.c_str(), std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("peek_file_size() : error opening file");

	// get its size:
	file.seekg(0, std::ios::end);
	file_size = file.tellg();
	file.seekg(0, std::ios::beg);

	file.close();
	return (size_t)file_size;
}

std::string itoa(int i)
{
	unsigned int j;
	std::string out;
	if (i < 0)
	{
		out = "-";
		j = -i;
	}
	else
		j = i;

	if (j / 10 != 0)
	{
		out.append(itoa((int)(j / 10)));
		out.push_back((char)(j % 10 + 48));
		return (out);
	}
	out = j + 48;
	return (out);
}

std::string http_index(std::string folder)
{
	std::string index;
	DIR *d;
	struct dirent *dir;
	struct stat result;
	std::string real_dir;
	d = opendir((real_dir.append(folder)).c_str());

	if (d != NULL)
	{
		index = filetostr("./preset_index.html");
		index.append("/");
		index.append(folder);
		index.append("/");
		index.append("</h1> <table id=\"list\" cellpadding=\"0.1em\" cellspacing=\"0\"> <colgroup> <col width=\"55%\"/> <col width=\"20%\"/> <col width=\"25%\"/> </colgroup> <thead> <tr><th><strong>Name</strong></th> <th><strong>File Size</strong></th> <th><strong>Date</strong></th> </thead> <tbody>");
		while ((dir = readdir(d)))
		{
			index.append("<tr><td><a href=\"");
			index.append(dir->d_name);
			index.append("\">");
			index.append(dir->d_name);
			index.append("</a></td><td>");

			real_dir = "";
			real_dir.append(folder);
			real_dir.append("/");

			if (dir->d_type == DT_REG && stat(real_dir.append(dir->d_name).c_str(), &result) == 0)
				index.append(itoa((int)result.st_size));
			else
				index.append("-");
			index.append("</td><td>");
			if (dir->d_type == DT_DIR && stat(real_dir.append(dir->d_name).c_str(), &result) == 0)
				index.append(get_gmt_time((result.st_mtime)));
			else
				index.append(get_gmt_time((result.st_mtime)));
			index.append("</td></tr>\n");
		}
		index.append("</tbody> </table> </div> </div> </body> </html>");
		closedir(d);
	}
	else
		perror("");
	
	return (index);
}

std::string		format_env_key(std::string x_key)
{
	for (auto& c : x_key)
		c = std::toupper(c);
	x_key.replace(0, 2, "HTTP_X_");
	return x_key;
}
