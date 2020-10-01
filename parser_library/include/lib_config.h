#ifndef HLASMPLUGIN_PARSERLIBRARY_LIB_CONFIG_H
#define HLASMPLUGIN_PARSERLIBRARY_LIB_CONFIG_H

#include <memory>

#include "json.hpp"

#include "parser_library_export.h"

struct PARSER_LIBRARY_EXPORT lib_config
{
	static std::shared_ptr<lib_config> get_instance();
	static void load_from_json(const nlohmann::json & config);

	bool continuationHandling;
};



#endif HLASMPLUGIN_PARSERLIBRARY_LIB_CONFIG_H