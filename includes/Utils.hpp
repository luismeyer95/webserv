#pragma once

#include "header.h"

typedef unsigned char BYTE;

bool check(bool expr, bool log, bool fatal, std::string);
std::vector<std::string> tokenizer( const std::string& str, char delim );
std::string filetostr( std::string filename );
std::vector<BYTE> readbin(const std::string& filename);

