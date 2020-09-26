#include <CGI.hpp>
#include <RequestRouter.hpp>

void CGI::executeCGI (
	FileRequest&		file_req,
	const std::map<EnvCGI, std::string>& env
)
{
	(void)file_req;
	(void)env;
}