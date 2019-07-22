#ifndef HLASMPLUGIN_LANGUAGESERVER_FEATURE_H
#define HLASMPLUGIN_LANGUAGESERVER_FEATURE_H

#include <map>
#include <string>

#include "json.hpp"

#include "common_types.h"
#include "shared/workspace_manager.h"

namespace hlasm_plugin {
namespace language_server {

class response_provider
{
public:
	virtual void respond(const json & id, const std::string & requested_method, const json & args) = 0;
	virtual void notify(const std::string & method, const json & args) = 0;
	virtual void respond_error(const json & id, const std::string & requested_method,
		int err_code, const std::string & err_message, const json & error) = 0;
};

class feature
{

public:
	feature(parser_library::workspace_manager & ws_mngr) : ws_mngr_(ws_mngr) {}
	feature(parser_library::workspace_manager & ws_mngr, response_provider & response_provider) : ws_mngr_(ws_mngr), response_(&response_provider) {}

	void virtual register_methods(std::map<std::string, method> & methods) = 0;
	json virtual register_capabilities() = 0;

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
	response_provider * response_ = nullptr;
};

}
}

#endif // !HLASMPLUGIN_LANGUAGESERVER_FEATURE_H
