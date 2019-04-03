#ifndef HLASMPLUGIN_PARSERLIBRARY_PARSE_LIB_PROVIDER_H
#define HLASMPLUGIN_PARSERLIBRARY_PARSE_LIB_PROVIDER_H

#include "context/hlasm_context.h"

namespace hlasm_plugin::parser_library
{

using parse_result = bool;

class parse_lib_provider
{
public:
	virtual parse_result parse_library(const std::string & library, context::ctx_ptr ctx) = 0;
	
};

class empty_parse_lib_provider : public parse_lib_provider
{
public:
	virtual parse_result parse_library(const std::string &, context::ctx_ptr) override { return true; };

	static empty_parse_lib_provider instance;
};


}

#endif //HLASMPLUGIN_PARSERLIBRARY_PARSE_LIB_PROVIDER_H