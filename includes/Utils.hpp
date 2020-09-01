#pragma once

#include "header.h"

typedef unsigned char BYTE;

bool 						check(bool expr, bool log, bool fatal, std::string);
std::vector<std::string>	tokenizer( const std::string& str, char delim );
std::string					filetostr( std::string filename );
std::vector<BYTE>			readbin(const std::string& filename);
void						http_print(const std::string& s);
void						dec_print(const char *s);
