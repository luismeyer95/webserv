#pragma once

#include "header.h"
#include "Utils.hpp"

#define BOLDBLACK "\e[1;30m"
#define BOLDRED "\e[1;31m"
#define BOLDGREEN "\e[1;32m"
#define BOLDYELLLOW "\e[1;33m"
#define BOLDBLUE "\e[1;34m"
#define BOLDMAGENTA "\e[1;35m"
#define BOLDCYAN "\e[1;36m"
#define BOLDWHITE "\e[1;37m"

#define WHITEBG "\e[47m"

#define RESET "\e[0m"

class Logger {
	private:
		bool				_log;
		std::string 		_logpath;
		std::ofstream		_file;

		Logger();

		std::string			getTime();

	public:
		~Logger();

		void					setLogPath(const std::string& logpath);
		std::ostream&			out(bool entry = true, bool timeheader = true);
		void					out(const std::string& str);

		void					hl(const std::string& header, const std::string& str = {});
		
		void					toggleLog();

		static Logger&	getInstance();
};