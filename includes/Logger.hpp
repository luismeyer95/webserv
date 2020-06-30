#pragma once

#include "header.h"
#include "Utils.hpp"

class Logger {
	private:
		bool				_log;
		std::string 		_logpath;
		std::ofstream		_file;
		std::stringstream	_errstr;

		Logger();

		std::string			getTime();

	public:
		~Logger();

		void					setLogPath(const std::string& logpath);
		std::stringstream&		err();
		std::ostream&			out(bool entry = true, bool timeheader = true);
		void					out(const std::string& str);
		bool					assert(bool expr, bool fatal);
		void					toggleLog();

		static Logger&	getInstance();
};