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

ConfDirective::ConfDirective(DirectiveKey key, const std::vector<std::string>& values)
	: key(key), values(values) {}

void ConfDirective::validate()
{
	using D = DirectiveKey;
	switch (key)
	{
		case D::listen: {
			if (values.empty())
				throw std::runtime_error (
					"ConfDirective: Empty value for `" + directiveKeyToString(key)
					+ "` directive is not accepted"
				);
			std::runtime_error oor (
				"ConfDirective: Out of range port number for `"
				+ directiveKeyToString(key) + "` directive"
			);
			int port;
			try {
				port = std::stoi(values[0]);
			} catch (const std::out_of_range& e) {
				throw oor;
			}
			if (port < 0 || port > 65535)
				throw oor;
			break;
		}
		case D::server_name: {
			// . . .
			break;
		}
		case D::root: {
			// . . .
			break;
		}
		case D::error_page: {
			// . . .
			break;
		}
		case D::internal: {
			// . . .
			break;
		}
		case D::index: {
			// . . .
			break;
		}
		case D::autoindex: {
			// . . .
			break;
		}
	}
}