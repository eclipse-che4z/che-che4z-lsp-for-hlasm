#ifndef HLASMPLUGIN_PARSER_HLASM_EVALUATION_CONTEXT_H
#define HLASMPLUGIN_PARSER_HLASM_EVALUATION_CONTEXT_H

#include "../processing/attribute_provider.h"
#include "../context/hlasm_context.h"
#include "../parse_lib_provider.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

struct evaluation_context
{
	context::hlasm_context& hlasm_ctx;
	processing::attribute_provider& attr_provider;
	parse_lib_provider& lib_provider;
};

}
}
}

#endif
