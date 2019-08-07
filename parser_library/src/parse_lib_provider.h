#ifndef HLASMPLUGIN_PARSERLIBRARY_PARSE_LIB_PROVIDER_H
#define HLASMPLUGIN_PARSERLIBRARY_PARSE_LIB_PROVIDER_H

#include "context/hlasm_context.h"

namespace hlasm_plugin::parser_library
{

using parse_result = bool;

struct library_data
{
	context::file_processing_type proc_type;
	context::id_index library_member;
};

class parse_lib_provider
{
public:
	virtual parse_result parse_library(const std::string & library, context::hlasm_context& hlasm_ctx, const library_data data) = 0;
	
};

class empty_parse_lib_provider : public parse_lib_provider
{
public:
	virtual parse_result parse_library(const std::string &, context::hlasm_context&, const library_data) override { return false; };

	static empty_parse_lib_provider instance;
};


}

#endif //HLASMPLUGIN_PARSERLIBRARY_PARSE_LIB_PROVIDER_H