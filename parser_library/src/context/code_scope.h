#ifndef CONTEXT_CODE_SCOPE_H
#define CONTEXT_CODE_SCOPE_H

#include "macro.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

//helper struct for hlasm code scopes
//contains locally valid set symbols, sequence symbols and pointer to macro class (if code is in any) 
struct code_scope
{
	using set_sym_storage = std::unordered_map<id_index, set_sym_ptr>;

	//local variables of scope
	set_sym_storage variables;
	//local sequence symbols of scope
	label_storage sequence_symbols;
	//gets macro to which this scope belong (nullptr if in open code)
	macro_invo_ptr this_macro;
	//the ACTR branch counter
	A_t branch_counter;

	//semantics::symbol_range current_stmt_range;
	
	bool is_in_macro() const { return !!this_macro; }

	code_scope(macro_invo_ptr macro_invo) : this_macro(std::move(macro_invo)), branch_counter(4096) {}
	code_scope() : branch_counter(4096) {}
};

}
}
}
#endif
