#include <Logger.hpp>
#include <ServerSocketPool.hpp>
#include <RequestParser.hpp>
#include <ResponseConstructor.hpp>
#include <Utils.hpp>
#include <header.h>
#include <ByteBuffer.hpp>
#include <SharedPtr.hpp>
#include <Regex.hpp>
#include <Conf.hpp>
#include <URL.hpp>

int main(int ac, char **av)
{	
	if (ac != 2)
	{
		std::cout << "usage: " << av[0] << " <configuration file>" << std::endl;
		return (0);
	}

	Logger& log = Logger::getInstance();
	std::vector<std::string> args(av, av + ac);

	ServerSocketPool pool;
	try {

		log.out() << "Initializing server..." << std::endl;
		log.out() << "Loading configuration file" << std::endl;

		Config conf(args[1]);
		log.hl(BOLDGREEN "SUCCESS");

		log.out() << "Setting up virtual hosts" << std::endl;
		RequestRouter router(conf);

		pool.setConfig(router);

	} catch (const std::runtime_error& e) {
		log.hl(BOLDRED "ERROR", BOLDWHITE + std::string(e.what()));
		return (1);
	}

	log.hl(BOLDGREEN "SERVER IS RUNNING...");
	pool.runServer();
}