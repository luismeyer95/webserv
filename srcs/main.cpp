#include <Logger.hpp>
#include <ServerSocketPool.hpp>
#include <RequestParser.hpp>
#include <Utils.hpp>
#include <header.h>
#include <ByteBuffer.hpp>

#include <Regex.hpp>
#include <Conf.hpp>
#include <URL.hpp>

void	handle_connection(HTTPExchange& comm, RequestRouter& router)
{
	(void)comm;
	(void)router;
}

void	handle_request(HTTPExchange& comm, RequestRouter& router)
{
	(void)router;

	Logger& log = Logger::getInstance();
	// Extracting the resource's path
	std::string msg(comm.request);
	std::string resource = tokenizer(msg, ' ')[1];

	//call RequestParser here

	//GET
	if (resource.find("/..") != std::string::npos)
		return;
	
	if (resource == "/")
		resource = "./simple_site/index.html";
	else
		resource = "./simple_site" + resource;

	// Buffering a generic response for all calls (simply sends 200 OK + the resource's content)
	ByteBuffer doc;
	doc << "HTTP/1.1 200 OK\r\n";
	doc << "Content-Length: " << ByteBuffer::peekFileSize(resource) << "\r\n\r\n";
	doc.appendFile(resource);

	// Load the response in the http exchange ticket and mark as ready
	comm.bufferResponse(doc, true);
}

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

		log.hl(BOLDGREEN "SUCCESS");

	} catch (const std::runtime_error& e) {
		log.hl(BOLDRED "ERROR", BOLDWHITE + std::string(e.what()));
		return (1);
	}

	log.hl(BOLDGREEN "SERVER IS RUNNING...");
	pool.runServer(handle_connection, handle_request);
}

// int main(int ac, char **av)
// {
// 	if (ac != 5)
// 		return 1;

// 	Config conf("./webserv.conf");

// 	RequestRouter router(conf);
// 	router.bindRequest (
// 		av[1], 				// request uri
// 		av[2],				// request server_name
// 		av[3],				// ip
// 		std::stoi(av[4])	// port
// 	);
// 	auto strs = router.getBoundRequestDirectiveValues(DirectiveKey::root);
// 	for (auto& s : strs)
// 		std::cout << s << std::endl;
// }


// int main(int ac, char **av)
// {
// 	// Creating a URL object from its non-encoded component parts
// 	URL url (
// 		/* scheme   */ "http",
// 		/* host	    */ "dev.webserv.net",
// 		/* port     */ "80",
// 		/* path	    */ "/path/Écologie: quels sont les enjeux?.html",
// 		/* query    */ "q=5",
// 		/* fragment */ "top"
// 	);
// 	std::cout << url.getFullURL() << std::endl;

// 	// Creating a URL object from an already encoded URL
// 	// This URL is equivalent to the previous one
// 	URL url_bis (
// 		"http://dev.webserv.net:80/path/%c3%89cologie:%20quels%20sont%20les%20enjeux%3f.html?q=5#top"
// 	);

// 	return 0;
// }


// int main(int ac, char **av)
// {
	// if (ac != 2)
	// 	return (1);
	// auto res = Regex(av[1]).match(av[2]);
	// std::cout << "Match: " << res.first << std::endl;
	// if (res.first)
	// 	std::cout << "String: " << res.second << std::endl;
// }