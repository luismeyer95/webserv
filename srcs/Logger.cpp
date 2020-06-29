#include "../includes/Logger.hpp"

Logger&	Logger::getInstance()
{
	static Logger instance;
	return instance;
}

Logger::Logger()
	: _log(true), _logpath(""), _file(), _errstr()
{}

Logger::~Logger()
{
	if (_file.is_open())
		_file.close();
}

void				Logger::setLogPath(const std::string& path)
{
	if (!path.empty())
	{
		_logpath = path;
		if (_file.is_open())
			_file.close();
		_file.open(path);
		if (!_file.is_open())
			std::cerr << "[Logger]: unable to open file in setLogPath(). (" <<
			__FILE__ << ", line " << __LINE__ << ")" << std::endl;
	}
}

std::stringstream&	Logger::err()
{
	return _errstr;
}

std::ostream&		Logger::out(bool entry, bool timeheader)
{
	std::ostream& o = _file.is_open() ? _file : std::cout;
	if (!entry)
		return o;
	std::string str;
	if (!timeheader)
		str = "";
	else
		str = getTime();
	o << std::setw(25) << std::left << str;
	o << std::setw(0);
	return o;
}

std::string				Logger::getTime()
{
	struct timeval tv;
	time_t t;
	struct tm *info;
	char buffer[64];

	gettimeofday(&tv, NULL);
	t = tv.tv_sec;
	info = localtime(&t);
	strftime(buffer, sizeof(buffer), "%D %Ih%Mm%Ss %p", info);
	return std::string(buffer);
}

bool				Logger::assert(bool expr, bool fatal)
{
	if (!expr)
	{
		if (_log)
		{
			std::string time = getTime();
			std::ostream& o = _file.is_open() ? _file : std::cerr;
			o << std::setw(25) << std::left << time << "[ERROR]: " <<_errstr.str() << std::endl;
			o << std::setw(0);
		}
		if (fatal)
			exit(1);
	}
	_errstr.str(std::string());
	return expr;
}

void				Logger::toggleLog()
{
	_log = !_log;
}