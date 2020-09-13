#include <header.h>

class ErrorCodeException : public std::exception
{
	private:
		int			err_code;
		std::string err_str;
	public:
		ErrorCodeException(int code, const std::string& str);
		
		std::string			str() const throw();
		int					code() const throw();
};
