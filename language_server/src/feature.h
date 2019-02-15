#ifndef HLASMPLUGIN_LANGUAGESERVER_FEATURE_H
#define HLASMPLUGIN_LANGUAGESERVER_FEATURE_H

#include <map>
#include <string>

#include "json.hpp"
#include "jsonrp.hpp"

#include "common_types.h"
#include "shared/workspace_manager.h"

namespace hlasm_plugin {
namespace language_server {

class feature
{

public:
	feature(parser_library::workspace_manager & ws_mngr) :ws_mngr_(ws_mngr) {}

	void virtual register_methods(std::map<std::string, method> & methods) = 0;
	void virtual register_notifications(std::map<std::string, notification> & notification) = 0;
	json virtual register_capabilities() = 0;
	void virtual register_callbacks(response_callback response, response_error_callback error, notify_callback notify) = 0;

	void virtual initialize_feature(const json & client_capabilities) = 0;
	
	static std::string uri_to_path(const std::string & uri);
	static std::string path_to_uri(std::string path);

	virtual ~feature() = default;

	static parser_library::range parse_range(const json & range_json);
	static parser_library::position parse_location(const json & location_json);

	static json range_to_json(parser_library::range range);
	static json location_to_json(parser_library::position location);
protected:
	parser_library::workspace_manager & ws_mngr_;
	bool callbacks_registered_ = false;
};

}
}

#endif // !HLASMPLUGIN_LANGUAGESERVER_FEATURE_H
