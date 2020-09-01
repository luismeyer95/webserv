#include "../includes/Utils.hpp"


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