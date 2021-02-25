#include "json_queue_channel.h"

namespace hlasm_plugin::language_server {

std::optional<nlohmann::json> hlasm_plugin::language_server::json_queue_channel::read() { return queue.pop(); }

void json_queue_channel::write(const nlohmann::json& json) { queue.push(json); }
void json_queue_channel::write(nlohmann::json&& json) { queue.push(std::move(json)); }

void json_queue_channel::terminate() { queue.terminate(); }

} // namespace hlasm_plugin::language_server