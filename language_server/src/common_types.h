#include "json.hpp"

namespace hlasm_plugin::language_server {

	using json = nlohmann::json;

	using method = std::function<void(const json & id, const json & params)>;

	using send_message_callback = std::function<void(const json &)>;
}
