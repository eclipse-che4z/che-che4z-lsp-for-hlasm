#ifndef CONTEXT_CODE_SCOPE_H
#define CONTEXT_CODE_SCOPE_H

#include "macro.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

//helper struct for HLASM code scopes
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
	//number of changed branch counters
	size_t branch_counter_change;
	
	bool is_in_macro() const { return !!this_macro; }

	code_scope(macro_invo_ptr macro_invo) 
		: this_macro(std::move(macro_invo)), branch_counter(4096), branch_counter_change(0) {}
	code_scope() 
		: branch_counter(4096), branch_counter_change(0) {}
};

}
}
}
#endif
