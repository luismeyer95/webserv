#pragma once

#include "header.h"

typedef unsigned char BYTE;

bool 						check(bool expr, bool log, bool fatal, std::string);
std::vector<std::string>	tokenizer( const std::string& str, char delim );
std::string					filetostr( std::string filename );
std::vector<BYTE>			readbin(const std::string& filename);
std::string					make_html_error_page(int error_code, const std::string& error_string);
void						http_print(const std::string& s);
void						dec_print(const char *s);
std::vector<std::string>    header_finder(const std::vector<std::string> lines, const std::string to_find);
std::vector<std::string>    get_header_name(std::string str, char c);
bool                        is_number(const std::string s);
bool                        is_http_date(std::string str);
std::string                 trim(const std::string &str);
bool                        check_str_len(const std::string &str, int min, int max);