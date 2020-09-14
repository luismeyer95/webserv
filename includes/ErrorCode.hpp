#pragma once

#include <header.h>

class ErrorCode : public std::exception
{
	private:
		int			err_code;
		std::string err_str;
	public:
		ErrorCode(int code, const std::string& str);
		
		std::string			str() const throw();
		int					code() const throw();
};
