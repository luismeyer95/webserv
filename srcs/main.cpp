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

void	handle_connection(HTTPExchange& comm, RequestRouter& router)
{
	(void)comm;
	(void)router;
}

void	handle_request(HTTPExchange& comm, RequestRouter& router)
{
	(void)router;

	Logger& log = Logger::getInstance();
	// Extracting header in 
	//call RequestParser here

	RequestParser request;
	request.parser(comm.request);

	FileRequest file_request(router.requestFile(request, comm));

	ResponseConstructor response;
	ByteBuffer headers = response.constructor(request, file_request);

	SharedPtr<ResponseBuffer> response_buffer(file_request.response_buffer);
	response_buffer->get().prepend(headers);
	// Load the response in the http exchange ticket and mark as ready
	comm.bufferResponse(headers, response_buffer, true);
}



int main(int ac, char **av)
{

	// std::cout << http_index(av[1]) << std::endl;
	// exit(0);
	std::cout << getpid() << std::endl;
	
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
	pool.runServer(handle_connection, handle_request);
}

// int main(int ac, char **av)
// {
// 	if (ac != 6)
// 		return 1;

// 	Config conf("./webserv.conf");

// 	RequestRouter router(conf);

// 	FileRequest fr = router.requestFile (
// 		av[1], 				// request uri
// 		av[2],				// request server_name
// 		av[3],				// ip
// 		std::stoi(av[4]),	// port
// 		av[5]				// basic_auth
// 	);

// 	std::cout << "code: " << fr.http_code << std::endl;
// 	std::cout << "msg: " << fr.http_string << std::endl;
// 	std::cout << "path: " << fr.file_path << std::endl;
// 	std::cout << "realm: " << fr.realm << std::endl;
// 	std::cout << "last modified: " << fr.last_modified << std::endl;
// 	std::cout << "content: " << fr.file_content.str() << std::endl;

// }


// int main(int ac, char **av)
// {
// 	// // Creating a URL object from its non-encoded component parts
// 	// URL url (
// 	// 	/* scheme   */ "http",
// 	// 	/* host	    */ "dev.webserv.net",
// 	// 	/* port     */ "80",
// 	// 	/* path	    */ "/path/Écologie: quels sont les enjeux?.html",
// 	// 	/* query    */ "q=5",
// 	// 	/* fragment */ "top"
// 	// );
// 	// std::cout << url.getFullURL() << std::endl;

// 	// // Creating a URL object from an already encoded URL
// 	// // This URL is equivalent to the previous one
// 	// URL url_bis (
// 	// 	"http://dev.webserv.net:80/path/%c3%89cologie:%20quels%20sont%20les%20enjeux%3f.html?q=5#top"
// 	// );

// 	(void)ac;
// 	URL url(av[1]);
// 	// std::cout << URL::decode(url.get(URL::Component::Path)) << std::endl;
// 	url.printComponents();
// 	return 0;
// }


// int main(int ac, char **av)
// {
// 	if (ac != 3)
// 		return (1);
// 	auto res = Regex(av[1]).matchAll(av[2]);
// 	std::cout << "Match: " << res.first << std::endl;
// 	if (res.first)
// 	{
// 		int i = 0;
// 		for (auto& s : res.second)
// 			std::cout << "[" << i++ << "]: " << s << std::endl;
// 	}

// 	// if (ac != 3)
// 	// 	return (1);
// 	// auto res = Regex(av[1]).match(av[2]);
// 	// std::cout << "Match: " << res.first << std::endl;
// 	// if (res.first)
// 	// {
// 	// 	std::cout << "String: " << res.second << std::endl;
// 	// }
// }