#pragma once

#include "header.h"

typedef unsigned char BYTE;

bool 						check(bool expr, bool log, bool fatal, std::string);
std::vector<std::string>	tokenizer( const std::string& str, char delim );
std::string					filetostr( std::string filename );
std::vector<BYTE>			readbin(const std::string& filename);
std::string					make_html_error_page(int error_code, const std::string& error_string);
std::string					get_gmt_time(time_t date);
void						http_print(const std::string& s);
void						dec_print(const char *s);

