#ifndef HLASMPLUGIN_LANGUAGESERVER_FEATURE_H
#define HLASMPLUGIN_LANGUAGESERVER_FEATURE_H

#include <map>
#include <string>

#include "json.hpp"
#include "jsonrp.hpp"

#include "shared/workspace_manager.h"

namespace hlasm_plugin {
namespace language_server {

using json = nlohmann::json;

using id = jsonrpcpp::Id;
using parameter = nlohmann::json;

using method = std::function<void(id, const parameter &)>;
using notification = std::function<void(const parameter &)>;



class feature
{

public:
	feature(parser_library::workspace_manager & ws_mngr) :ws_mngr_(ws_mngr) {}

	void virtual register_methods(std::map<std::string, method> & methods) = 0;
	void virtual register_notifications(std::map<std::string, notification> & notification) = 0;
	json virtual register_capabilities() = 0;

	void virtual initialize_feature(const json & client_capabilities) = 0;
	
	static std::string uri_to_path(const std::string & uri);

	virtual ~feature() = default;

	static parser_library::range parse_range(const json & range_json);
	static parser_library::location parse_location(const json & location_json);
protected:
	parser_library::workspace_manager & ws_mngr_;
};

}
}

#endif // !HLASMPLUGIN_LANGUAGESERVER_FEATURE_H
