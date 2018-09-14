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
#ifdef _WIN32
		auth_path = "//" + auth_path;
#endif
	}
	else
	{
#ifdef _WIN32
		path.remove_prefix(1);
#endif
		auth_path = path.to_string();
	}

	std::filesystem::path p(network::detail::decode(auth_path));
	return p.lexically_normal().string();
}

parser_library::range feature::parse_range(const json & range_json)
{
	return { parse_location(range_json["start"]),
		parse_location(range_json["end"]) };
}

parser_library::location feature::parse_location(const json & location_json)
{
	return { location_json["line"].get<nlohmann::json::number_unsigned_t>(),
		location_json["character"].get<nlohmann::json::number_unsigned_t>() };
}

}
