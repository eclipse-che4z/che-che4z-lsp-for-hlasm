#ifndef CONTEXT_ALIGNMENT_H
#define CONTEXT_ALIGNMENT_H

namespace hlasm_plugin {
namespace parser_library {
namespace context {

//structure representing required alignment of storage area
template<size_t byte , size_t boundary>
struct alignment
{
	static_assert(byte < boundary && byte % 2 == 0);
};

//enumeration of common alignments
static constexpr alignment<0, 1> no_align{};
static constexpr alignment<0, 2> halfword{};
static constexpr alignment<0, 4> word{};
static constexpr alignment<0, 8> doubleword{};
static constexpr alignment<0, 16> quadword{};

}
}
}
#endif
