#include <filesystem>

#include "network/uri/uri.hpp"
#include "logger.h"
#include "feature.h"

namespace hlasm_plugin::language_server {

std::string feature::uri_to_path(const std::string & uri)
{ 
	network::uri u(uri);
	
	if (u.scheme().compare("file"))
		return "";
	if (!u.has_path())
		return "";
	network::string_view path = u.path();



	std::string auth_path;
	if (u.has_authority() && u.authority().to_string() != "")
	{
		auth_path = u.authority().to_string() + u.path().to_string();
#ifdef _WIN32 //handle remote locations correctly, like \\server\path
		auth_path = "//" + auth_path;
#endif
	}
	else
	{
#ifdef _WIN32 //we get path always beginning with / on windows, e.g. /c:/Users/path
		path.remove_prefix(1);
#endif
		auth_path = path.to_string();

#ifdef _WIN32
		auth_path[0] = tolower(auth_path[0]);
#endif
	}

	std::filesystem::path p(network::detail::decode(auth_path));
	return p.lexically_normal().string();
}

std::string feature::path_to_uri(std::string path)
{
	std::replace(path.begin(), path.end(), '\\', '/');
	// network::detail::encode_path(uri) ignores @, which is incompatible with VS Code
	std::string uri;
	auto out = std::back_inserter(uri);
	auto it = path.cbegin();
	while (it != path.cend()) {
		network::detail::encode_char(*it, out, "/.%;=");
		++it;
	}

#ifdef _WIN32
	//in case of remote address such as \\server\path\to\file
	if(uri.size() >= 2 && uri[0] == '/' && uri[1] == '/')
		uri.insert(0, "file:");
	else
		uri.insert(0, "file:///");
#else
	uri.insert(0, "file://");
#endif // _WIN32

	return uri;
}

parser_library::range feature::parse_range(const json & range_json)
{
	return { parse_location(range_json["start"]),
		parse_location(range_json["end"]) };
}

parser_library::position feature::parse_location(const json & location_json)
{
	return { location_json["line"].get<nlohmann::json::number_unsigned_t>(),
		location_json["character"].get<nlohmann::json::number_unsigned_t>() };
}

json feature::range_to_json(parser_library::range range)
{
	return json{ {"start", location_to_json(range.start)}, {"end", location_to_json(range.end)} };
}

json feature::location_to_json(parser_library::position location)
{
	return json{ {"line", location.line}, {"character", location.column} };
}

}
