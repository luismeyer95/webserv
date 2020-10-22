#pragma once

#include "header.h"
#include <URL.hpp>

typedef unsigned char BYTE;

bool 						check(bool expr, bool log, bool fatal, std::string);
std::vector<std::string>	tokenizer( const std::string& str, char delim );
std::vector<std::string>	real_tokenizer( const std::string& str, char delim );
std::vector<std::string> 	strsplit( const std::string& str, const std::string& delim_set);
std::string					filetostr( std::string filename );
std::vector<BYTE>			readbin(const std::string& filename);
std::string					make_html_error_page(int error_code, const std::string& error_string);
std::string					get_gmt_time(time_t date);
void						http_print(const std::string& s);
void						dec_print(const char *s);
std::vector<std::string>    header_finder(const std::vector<std::string> lines, const std::string to_find);
std::vector<std::string>    get_header_name(std::string str, char c);
bool                        is_number(const std::string s);
bool                        is_http_date(std::string str);
std::string                 trim(const std::string &str);
bool                        check_str_len(const std::string &str, int min, int max);
std::string					reformat_path(std::string input_path);
std::string 				join_paths(const std::string& root, const std::string& rel);
std::string                 get_http_code(int i);
std::string					get_http_string(int code);
std::string					get_cur_dir();
void						set_current_dir(const std::string& path);
std::string					get_parent_dir(std::string script_path);
std::string					http_index(std::string root, std::string relative_part, std::string request_uri);
std::string                 itoa(int i);
std::map<std::string,
std::vector<std::string>>& 	mime_types();
std::string					get_mime_type(const std::string& path);
size_t						peek_file_size(const std::string& filename);
std::string					ntohexstr(size_t num);
std::string					format_env_key(std::string x_key);
std::string					preset_index_html();
std::string					strtoupper(std::string s);