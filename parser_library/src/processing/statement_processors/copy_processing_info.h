#ifndef PROCESSING_COPY_PROCESSING_INFO_H
#define PROCESSING_COPY_PROCESSING_INFO_H

#include "../../context/hlasm_statement.h"
#include "../../context/id_storage.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//data to start copy_processor
struct copy_start_data
{
	context::id_index member_name;
};

//result of copy_processor
struct copy_processing_result
{
	context::statement_block definition;
	location definition_location;
	context::id_index member_name;

	bool invalid_member;
};

}
}
}
#endif
