#ifndef PROCESSING_STATEMENT_PROVIDER_KIND_H
#define PROCESSING_STATEMENT_PROVIDER_KIND_H

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//kind of statement providers
enum class statement_provider_kind
{
	MACRO = 0, COPY=1, OPEN = 2 //MACRO = 0, AINS = 1, COPY = 2, OPEN = 3 -- priority
};

}
}
}
#endif
