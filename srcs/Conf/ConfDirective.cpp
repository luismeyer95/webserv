#include <Conf/ConfDirective.hpp>

std::map<std::string, DirectiveKey> directiveKeyLookup()
{
	using D = DirectiveKey;
	return std::map<std::string, DirectiveKey>
	({
		{"listen", D::listen},
		{"server_name", D::server_name},
		{"root", D::root},
		{"error_page", D::error_page},
		{"internal", D::internal},
		{"index", D::index},
		{"autoindex", D::autoindex}
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
		case D::error_page: return "error_page";
		case D::internal: return "internal";
		case D::index: return "index";
		case D::autoindex: return "autoindex";
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

			auto strs = tokenizer(values.at(0), ':');
			auto ipv4 = Regex (
				"^(([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])\\.){3}([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])$"
			).match(strs.at(0));
			if (!ipv4.first && strs.at(0) != "localhost")
				throw std::runtime_error
					("<host> component should either be a valid ipv4 address or `localhost`");

			ConfError oor = dirExcept("out of range <port> number");
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
			auto it = values.begin();
			for (;it != values.end(); ++it)
			{
				if (*it == "~" || *it == "~*")
				{
					++it;
					if (it == values.end())
						throw dirExcept("missing regex pattern after tilde specifier");
					try {
						Regex rgx(*it);
					}
					catch (const std::runtime_error& e) {
						throw dirExcept("invalid regex pattern");
					}
				}
				else
				{
					// need to assert validity of the syntax
					// try {
					// 	URL name;
					// 	name.get(URL::Component::Host) = 
					// }
				}		
			}
			break;
		}

		case D::root:
		{
			if (values.empty())
				throw dirExcept("missing value(s)");
			bool valid_uri = Regex("^/|(/[-_a-zA-Z\\d]+(\\.[-_a-zA-Z\\d]+)?)+/?$").match(values[0]).first;
			if (!valid_uri)
			{
				throw ConfError (
					line_nb,
					"invalid path syntax"
				);
			}
			break;
		}

		case D::error_page:
		{
			// . . .
			break;
		}

		case D::internal:
		{
			// . . .
			break;
		}

		case D::index:
		{
			// . . .
			break;
		}

		case D::autoindex:
		{
			// . . .
			break;
		}
	}
}