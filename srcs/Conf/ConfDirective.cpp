#include <Conf/ConfBlockDirective.hpp>
#include <Conf/ConfDirective.hpp>

std::map<std::string, DirectiveKey> directiveKeyLookup()
{
	using D = DirectiveKey;
	return std::map<std::string, DirectiveKey>
	({
		{"listen", D::listen},
		{"server_name", D::server_name},
		{"root", D::root},
		{"alias", D::alias},
		{"error_page", D::error_page},
		{"internal", D::internal},
		{"index", D::index},
		{"autoindex", D::autoindex},
		{"auth_basic", D::auth_basic},
		{"auth_basic_user_file", D::auth_basic_user_file},
		{"execute_cgi", D::execute_cgi},
		{"cgi_script_name", D::cgi_script_name},
		{"cgi_path_info", D::cgi_path_info},
		{"accept_methods", D::accept_methods},
		{"max_request_body", D::max_request_body},
		{"set_dir", D::set_dir},
		{"variant_search", D::variant_search}
	});
}

std::string directiveKeyToString(DirectiveKey key)
{
	using D = DirectiveKey;
	switch (key)
	{
		case D::listen: return "listen";
		case D::server_name: return "server_name";
		case D::root: return "root";
		case D::alias: return "alias";
		case D::error_page: return "error_page";
		case D::internal: return "internal";
		case D::index: return "index";
		case D::autoindex: return "autoindex";
		case D::auth_basic: return "auth_basic";
		case D::auth_basic_user_file: return "auth_basic_user_file";
		case D::execute_cgi: return "execute_cgi";
		case D::cgi_script_name: return "cgi_script_name";
		case D::cgi_path_info: return "cgi_path_info";
		case D::accept_methods: return "accept_methods";
		case D::max_request_body: return "max_request_body";
		case D::set_dir: return "set_dir";
		case D::variant_search: return "variant_search";
	}
}

ConfDirective::ConfDirective (
	int line_nb, DirectiveKey key,
	const std::vector<std::string>& values
) : line_nb(line_nb), parent(nullptr), key(key), values(values) {}

ConfError ConfDirective::dirExcept(const std::string& err)
{
	return ConfError (
		line_nb,
		err + " (`" + directiveKeyToString(key)
		+ "` directive)"
	);
}

ConfDirective*	ConfDirective::searchDirective(ConfBlockDirective *b, DirectiveKey key)
{
	ConfBlockDirective* tmp = b;
	while (tmp)
	{
		auto it = std::find_if (
			tmp->directives.begin(),
			tmp->directives.end(),
			[&] (ConfDirective dir) { return dir.key == key; }
		);
		if (it != tmp->directives.end())
			return &(*it);
		tmp = tmp->parent;
	}
	return nullptr;
}

std::string					ConfDirective::getSetDir()
{
	auto servroot_dir = searchDirective(parent, DirectiveKey::set_dir);
	if (servroot_dir == nullptr)
		return running_dir;
	else
	{
		servroot_dir->validate();
		return servroot_dir->values.at(0);
	}
}

void ConfDirective::validate()
{
	using D = DirectiveKey;
	switch (key)
	{
		case D::listen:
		{
			if (values.empty())
				throw dirExcept("missing value(s)");
			size_t col = values.at(0).find(":");
			if (col == std::string::npos)
				throw dirExcept("value should be formatted as follows : <host>:<port>");

			// auto strs = tokenizer(values.at(0), ':');
			auto strs = strsplit(values.at(0), ":");
			auto ipv4 = Regex (
				"^(([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])\\.){3}([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])$"
			).match(strs.at(0));
			if (!ipv4.first && strs.at(0) != "localhost")
				throw dirExcept
					("host component should either be a valid ipv4 address or `localhost`");

			ConfError oor = dirExcept("out of range port number");
			int port;
			try {
				port = std::stoi(strs.at(1));
			} catch (const std::out_of_range& e) {
				throw oor;
			}
			if (port < 0 || port > 65535)
				throw oor;
			break;
		}

		case D::server_name:
		{
			if (values.empty())
				throw dirExcept("missing value(s)");
			break;
		}

		case D::root:
		{
			if (values.empty())
				throw dirExcept("missing value(s)");

			auto servroot = getSetDir();

			struct stat buffer;
			std::string path = servroot + values.at(0);

			if (stat(path.c_str(), &buffer) != 0)
				throw dirExcept("path doesn't exist");
			else if (!(buffer.st_mode & S_IFDIR))
				throw dirExcept("path should point to a directory");

			break;
		}

		case D::alias:
		{
			if (values.empty())
				throw dirExcept("missing value(s)");

			auto res = Regex("^(.*[^\\\\])?(\\$\\d+)([^\\d].*)?$").match(values.at(0));
			if (res.first)
			{
				ConfBlockDirective& block = *parent;
				if (!(block.key == ContextKey::location && block.prefixes.at(0) == "~"))
					throw dirExcept (
						"`alias` directives containing capture variables"
						" are only valid inside regex-prefixed `location` block directives"
					);
			}

			break;
		}

		case D::error_page:
		{
			if (values.empty())
				throw dirExcept("missing value(s)");
			
			auto it = values.begin();
			for (; it != values.end(); ++it)
			{
				auto res = Regex("^\\d{3}$").match(*it);
				if (!res.first)
					break;
			}
			if (it == values.end())
				throw dirExcept("missing value(s)");
			auto res = Regex("^/.*xxx[^/]*").match(*it);
			if (!res.first)
				throw dirExcept("relative path must start with a `/` and "
					"contain a `xxx` substring for error code substitution");
			std::string uri = *it;
			size_t xxx = uri.find("xxx");
			uri.erase(xxx, 3);
			auto servroot = getSetDir();
			uri = servroot + uri;
			xxx += servroot.size();
			for (auto itb = values.begin(); itb != it; ++itb)
			{
				struct stat buffer;
				std::string path = uri;
				path.insert(xxx, *itb);
				if (stat(path.c_str(), &buffer) != 0)
					throw dirExcept("path `" + path + "` doesn't exist");
				else if (!(buffer.st_mode & S_IFREG))
					throw dirExcept("path should point to a file");
			}
			break;
		}

		case D::index:
		{
			if (values.empty())
				throw dirExcept("missing value(s)");
			break;
		}

		case D::internal: break;

		case D::autoindex:
		{
			if (values.empty())
				throw dirExcept("missing value(s)");
			if ((values.at(0) != "on" && values.at(0) != "off") || values.size() != 1)
				throw dirExcept("invalid value(s)");

			break;
		}

		case D::auth_basic:
		{
			if (values.empty())
				throw dirExcept("missing value(s)");
			ConfBlockDirective* b = parent;
			while (b)
			{
				size_t c = std::count_if(b->directives.begin(), b->directives.end(),
				[] (const ConfDirective& dir) { 
					return dir.key == DirectiveKey::auth_basic_user_file;
				});
				if (c == 1)
					return;
				else if (c > 1)
					throw dirExcept("conflicting `auth_basic_user_file` directives");
				b = b->parent;
			}
			throw dirExcept("missing an associated `auth_basic_user_file` directive");
			break;
		}

		case D::auth_basic_user_file:
		{
			if (values.empty())
				throw dirExcept("missing value(s)");

			if (values.size() > 1)
				throw dirExcept("excess values");
			
			struct stat buffer;
			std::string path = getSetDir() + values.at(0);
			if (stat(path.c_str(), &buffer) != 0)
				throw dirExcept("path doesn't exist");
			else if (!(buffer.st_mode & S_IFREG))
				throw dirExcept("path should point to a file");

			std::ifstream in(path);
			std::vector<std::string> userpass;
			if (in.is_open())
			{
				std::string line;
				Regex base64("^[\\da-zA-Z\\+\\/=]+$");
				while (getline(in, line, '\n'))
				{
					if (line.empty())
						continue;
					auto tokens = strsplit(line, ":");
					if (tokens.size() != 2)
						throw dirExcept ("`" + path + "` "
							"user-pass file entries require the following format: "
							"<user> ':' base64(<user> ':' <password>)"
						);
					if (!base64.match(tokens.at(1)).first)
						throw dirExcept ( "passwords in `" + path + "` "
							"must be encoded in base64"
						);
					userpass.push_back(line);
				}
				in.close();
				if (userpass.empty())
					throw dirExcept("`" + path + "` user-pass file is empty");
				values = std::move(userpass);
			}
			else
				throw dirExcept("could not open file `" + path + "`");

			break;
		}

		case D::execute_cgi:
		{
			if (values.empty())
				throw dirExcept("missing value(s)");
			if (parent->key != ContextKey::location)
				throw dirExcept("`execute_cgi` directives belong in `location` blocks");
			auto count_scriptname = std::count_if (
				parent->directives.begin(), parent->directives.end(),
				[] (const ConfDirective& d) {return d.key == DirectiveKey::cgi_script_name;}
			);
			auto count_pathinfo = std::count_if (
				parent->directives.begin(), parent->directives.end(),
				[] (const ConfDirective& d) {return d.key == DirectiveKey::cgi_path_info;}
			);

			if (!(count_scriptname == 1 && count_pathinfo == 1))
				throw dirExcept (
					"`cgi_script_name` and `cgi_path_info` directives should be defined once"
					" in the same block along with this directive"
				);

			break;
		}

		case D::cgi_script_name:
		{
			if (values.empty())
				throw dirExcept("missing value(s)");

			if (parent->key != ContextKey::location)
				throw dirExcept("directive belongs in `location` blocks");
			
			auto count = std::count_if (
				parent->directives.begin(), parent->directives.end(),
				[] (const ConfDirective& d) {
					return d.key == DirectiveKey::execute_cgi;
				}
			);

			if (count != 1)
				throw dirExcept (
					"directive should be coupled with "
					"a single `execute_cgi` directive"
				);
			
			break;
		}

		case D::cgi_path_info:
		{
			if (values.empty())
				throw dirExcept("missing value(s)");

			if (parent->key != ContextKey::location)
				throw dirExcept("directive belongs in `location` blocks");
			
			auto count = std::count_if (
				parent->directives.begin(), parent->directives.end(),
				[] (const ConfDirective& d) {
					return d.key == DirectiveKey::execute_cgi;
				}
			);

			if (count != 1)
				throw dirExcept (
					"directive should be coupled with "
					"a single `execute_cgi` directive"
				);
			
			break;
		}

		case D::accept_methods:
		{
			if (values.empty())
				throw dirExcept("missing value(s)");
			
			Regex rgx_method("^GET|HEAD|POST|PUT|all$");
			for (auto& s : values)
			{
				if (!rgx_method.match(s).first)
					throw dirExcept("not a supported method (`" + s + "`)");
			}

			if (std::find(values.begin(), values.end(), "all") != values.end())
			{
				values.clear();
				values.reserve(4);
				values.push_back("GET");
				values.push_back("HEAD");
				values.push_back("PUT");
				values.push_back("POST");
			}

			break;
		}

		case D::max_request_body:
		{
			if (values.empty())
				throw dirExcept("missing value(s)");
			
			Regex rgx("^\\d+$");
			auto res = rgx.match(values.at(0));
			if (!res.first)
				throw dirExcept("value must be a number");
			try {
				std::stoull(values.at(0));
			} catch (const std::out_of_range& e) {
				throw dirExcept("value must fit in a 64-bit unsigned integer");
			}
			break;
		}

		case D::set_dir:
		{
			if (values.empty())
				throw dirExcept("missing value(s)");
			
			std::string path = values.at(0);
			struct stat buffer;
			if (stat(path.c_str(), &buffer) != 0)
				throw dirExcept("path must exist and be either absolute or relative");
			else if (!(buffer.st_mode & S_IFDIR))
				throw dirExcept("path should point to a directory");
			break;
		}

		case D::variant_search:
		{
			if (values.empty())
				throw dirExcept("missing value(s)");
			
			if (!(values.at(0) == "on" || values.at(0) == "off"))
				throw dirExcept("directive can only have values `on` and `off`");

			break;
		}
	}
}