#include <Utils.hpp>

std::string filetostr( std::string filename )
{
	std::ostringstream dosString( std::ios::out | std::ios::binary ) ; // *** binary
	std::ifstream inFile( filename.c_str() ) ;

	std::string line;
	while( std::getline(inFile, line) )
		dosString << line << "\n" ;

	return dosString.str();
}

std::vector<std::string> tokenizer( const std::string& str, char delim )
{
	std::vector<std::string> tokens;
	std::stringstream   stream(str);
	std::string         temp;

	while(getline(stream, temp, delim ))
		tokens.push_back(temp);
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
	getline(stream, temp, '\n');
	out.push_back(temp);
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
        if (out[0] == to_find)
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
	getline(stream, temp, '\n');
	out.push_back(temp);
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
        if (out[0] == to_find)
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

bool check_str_len(const std::string &str, int min, int max)
{
	if (str.length() < min || str.length() > max)
		return (0);
}

bool is_http_date(std::string str)
{
	std::vector<std::string> tmp;
	std::string day_name[] = {"Mon,","Tue,", "Wed,","Thu,", "Fri,", "Sat,", "Sun,"};
	std::string month_name[] = {"Jan","Feb", "Mar","Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	tmp = tokenizer(str, ',');
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
	tmp = tokenizer(tmp[4], ':');
	if (tmp.size() != 3 || tmp[0].length() != 2 || !is_number(tmp[0]) ||tmp[1].length() != 2 || !is_number(tmp[1]) ||tmp[2].length() != 2 || !is_number(tmp[2]))
		return (0);
	return (1);
}