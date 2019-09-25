#ifndef CONTEXT_ALIGNMENT_H
#define CONTEXT_ALIGNMENT_H

#include <cstddef>

namespace hlasm_plugin::parser_library::context {

//structure representing required alignment of storage area
//boundary represents actual alignment
//byte is offset that is added to aligned location
struct alignment
{
	size_t byte;
	size_t boundary;
};

//enumeration of common alignments
static constexpr alignment no_align{ 0, 1 };
static constexpr alignment halfword{ 0, 2 };
static constexpr alignment fullword{ 0, 4 };
static constexpr alignment doubleword{ 0, 8 };
static constexpr alignment quadword{ 0, 16 };

}

#endif
