#ifndef PROCESSING_MACRODEF_PROCESSING_INFO_H
#define PROCESSING_MACRODEF_PROCESSING_INFO_H

#include "../../context/macro.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

struct macrodef_start_data {};

struct macrodef_prototype
{
	macrodef_prototype()
		:macro_name(context::id_storage::empty_id), name_param(context::id_storage::empty_id) {}

	context::id_index macro_name;

	context::id_index name_param;
	std::vector<context::macro_arg> symbolic_params;
};

struct macrodef_processing_result
{
	macrodef_prototype prototype;

	context::statement_block definition;
	context::label_storage sequence_symbols;

	location definition_location;
};

}
}
}
#endif
