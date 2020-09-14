#include <ErrorCode.hpp>

ErrorCode::ErrorCode(int code, const std::string& str)
	: err_code(code), err_str(str) {}

std::string ErrorCode::str() const throw()
{
	return err_str;
}

int ErrorCode::code() const throw()
{
	return err_code;
}