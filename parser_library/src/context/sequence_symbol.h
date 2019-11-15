#ifndef CONTEXT_SEQUENCE_SYMBOL_H
#define CONTEXT_SEQUENCE_SYMBOL_H

#include <memory>

#include "source_snapshot.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

struct sequence_symbol;
struct opencode_sequence_symbol;
struct macro_sequence_symbol;
using sequence_symbol_ptr = std::unique_ptr<sequence_symbol>;

enum class sequence_symbol_kind
{
	OPENCODE, MACRO
};

//structure representing sequence symbol
struct sequence_symbol
{
	//unique identifier
	id_index name;
	//location of the symbol in code
	location symbol_location; //TODO for lsp
	sequence_symbol_kind kind;

	const macro_sequence_symbol* access_macro_symbol() const;
	const opencode_sequence_symbol* access_opencode_symbol() const;

	virtual ~sequence_symbol() = default;

protected:
	sequence_symbol(id_index name, const sequence_symbol_kind kind, location symbol_location);
};

//struct representing sequence symbol that is in open-code
struct opencode_sequence_symbol : public sequence_symbol
{
	//position in opencode
	source_position statement_position;
	//snapshot of source for recovery
	source_snapshot snapshot;

	opencode_sequence_symbol(id_index name, location symbol_location, source_position statement_position, source_snapshot snapshot);

	bool operator==(const opencode_sequence_symbol& oth) const;
};

//struct representing sequence symbol that is in macro code
struct macro_sequence_symbol : public sequence_symbol
{
	//offset from start of the macro
	size_t statement_offset;

	macro_sequence_symbol(id_index name, location symbol_location, size_t statement_offset);
};

}
}
}

#endif