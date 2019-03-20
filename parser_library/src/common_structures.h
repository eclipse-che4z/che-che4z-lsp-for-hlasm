#ifndef HLASMPARSER_PARSERLIBRARY_COMMON_STRUCTURES_H
#define HLASMPARSER_PARSERLIBRARY_COMMON_STRUCTURES_H

namespace hlasm_plugin {
namespace parser_library {


//struct representing location of symbols
struct location
{
	size_t line;
	size_t offset;
};

}
}

#endif
