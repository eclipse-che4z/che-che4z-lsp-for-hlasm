#include "lib_config.h"

static std::shared_ptr<lib_config> config_instance;

std::shared_ptr<lib_config> lib_config::get_instance()
{
	return std::atomic_load(&config_instance);
}

void lib_config::load_from_json(const nlohmann::json& config)
{
	std::shared_ptr<lib_config> loaded = std::make_shared<lib_config>();

	auto found = config.find("continuationHandling");
	if (found != config.end())
		loaded->continuationHandling = found->get<bool>();

	std::atomic_store(&config_instance, loaded);
}