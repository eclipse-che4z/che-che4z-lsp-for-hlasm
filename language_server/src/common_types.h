
#include "jsonrp.hpp"
#include "json.hpp"

namespace hlasm_plugin::language_server {

	using json = nlohmann::json;

	using id = jsonrpcpp::Id;
	using parameter = nlohmann::json;

	using method = std::function<void(id, const parameter &)>;
	using notification = std::function<void(const parameter &)>;

	using error = jsonrpcpp::Error;
	//                     void reply(ID id, Json result, Json error)
	// mby take string instead of json? the json is deserialized in the next step anyway
	using response_callback = std::function<void(id, const Json&)>;
	using response_error_callback = std::function<void(id, const error&)>;
	using notify_callback = std::function<void(const std::string &, const Json&)>;



}