#include <ErrorCodeException.hpp>

ErrorCodeException::ErrorCodeException(int code, const std::string& str)
	: err_code(code), err_str(str) {}

std::string ErrorCodeException::str() const throw()
{
	return err_str;
}

int ErrorCodeException::code() const throw()
{
	return err_code;
}