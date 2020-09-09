#include <Logger.hpp>

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


// Prepares the indentation/format of the log stream for an entry given the parameters
// and returns the log stream. Timestamped entry is the default (log.out(true, true))
// - entry: preindents the current line
// - timeheader: timestamps the current line in the indentation space
// Ex: log.out() << "this is a timestamped log entry\n";
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

// Outputs the string str to the log stream.
// String is broken into line tokens and each line is printed
// individually to keep format consistency (indentation)
void		Logger::out(const std::string& str)
{
	std::ostream& o = _file.is_open() ? _file : std::cout;
	std::vector<std::string> tokens = tokenizer(str, '\n');
	std::string s;
	for (size_t i = 0; i < tokens.size(); ++i)
	{
		o << std::setw(25) << std::left << "";
		o << std::setw(0);
		o << tokens[i] << std::endl;
	}
}

void		Logger::ok()
{
	out() << "[" BOLDGREEN "OK" RESET "]" << std::endl;
}

void		Logger::error(const std::string& err)
{
	out() << "[" BOLDRED "ERROR" RESET "]: " << err << std::endl;
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

// Returns the error buffer stream
std::stringstream&	Logger::err()
{
	return _errstr;
}

// Asserts that the expression expr is true.
// - expr == true: nothing happens
// - expr == false: the buffered error stream's content is output to the log stream
//			 and the error stream is cleared. if fatal == true, function throws
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
			throw std::runtime_error("");
	}
	_errstr.str(std::string());
	return expr;
}

void				Logger::toggleLog()
{
	_log = !_log;
}