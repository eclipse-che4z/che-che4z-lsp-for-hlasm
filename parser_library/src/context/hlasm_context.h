#ifndef CONTEXT_HLASM_CONTEXT_H
#define CONTEXT_HLASM_CONTEXT_H

#include <memory>
#include <stack>

#include "code_scope.h"
#include "id_storage.h"
#include "macro.h"
#include "../diagnosable_impl.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {


//class helping to perform semantic analysis of hlasm source code
//wraps all classes and structures needed by semantic analysis (like variable symbol tables, opsyn tables...) in one place
//contains methods that store gathered information from semantic analysis helping it to correctly evaluate parsed code 
class hlasm_context : public diagnosable_impl
{

	using macro_storage = std::unordered_map<id_index, macro_definition>;
	using literal_map = std::unordered_map<id_index, id_index>;

	//storage of global variables
	code_scope::set_sym_storage globals_;
	//stack of nested scopes
	std::stack<code_scope> scope_stack_;

	//storage of defined macros
	macro_storage macros_;
	//map of OPSYN mnemotechnics
	literal_map opcode_mnemo_;

	code_scope* curr_scope();

public:

	hlasm_context();

	//storage for identifiers
	id_storage ids;

	//represents value of empty identifier
	const id_index empty_id;

	//return map of global set vars
	const code_scope::set_sym_storage& globals() const;

	//return variable symbol in current scope
	//returns empty shared_ptr if there is none
	var_sym_ptr get_var_sym(id_index name);

	void add_sequence_symbol(sequence_symbol seq_sym);

	sequence_symbol get_sequence_symbol(id_index name);

	void set_branch_counter(A_t value);

	A_t get_branch_counter();

	void decrement_branch_counter();

	//adds opsyn mnemonic
	void add_mnemonic(id_index mnemo, id_index op_code);

	//removes opsyn mnemonic
	void remove_mnemonic(id_index mnemo);

	//gets target of possible mnemonic changed by opsyn
	//returns nullptr if there is no mnemonic
	//returns index to default string ("") if opcode was deleted by opsyn
	//returns anything else if opcode was changed by opsyn
	id_index get_mnemonic_opcode(id_index mnemo) const;

	//creates specified global set symbol
	template <typename T>
	set_sym_ptr create_global_variable(id_index id, bool is_scalar)
	{
		//TODO error handling
		//if there is symbolic param with same name
		// add_diagnostic(diagnostic_s::error_E031("", "symbolic parameter", {})); //error - symbolic parameter with the same name
		//if there is set symbol with same name
		// add_diagnostic(diagnostic_s::error_E031("", "set symbol", {})); //error - set symbol with the same name


		auto tmp = curr_scope()->variables.find(id);
		if (tmp != curr_scope()->variables.end())
			return tmp->second;

		auto glob = globals_.find(id);
		if (glob != globals_.end())
		{
			curr_scope()->variables.insert({ id,glob->second });
			return glob->second;
		}

		set_sym_ptr val(std::make_shared<set_symbol<T>>(id, is_scalar));

		globals_.insert({ id,val });
		curr_scope()->variables.insert({ id,val });

		return val;
	}

	//creates specified local set symbol
	template <typename T>
	set_sym_ptr create_local_variable(id_index id, bool is_scalar)
	{
		auto tmp = curr_scope()->variables.find(id);
		if (tmp != curr_scope()->variables.end())
			return tmp->second;


		set_sym_ptr val(std::make_shared<set_symbol<T>>(id, is_scalar));

		curr_scope()->variables.insert({ id,val });

		return val;
	}

	bool is_in_macro() const;

	//returns macro we are currently in or empty shared_ptr if in open code
	macro_invo_ptr this_macro();

	const macro_definition& add_macro(id_index name, id_index label_param_name, std::vector<macro_arg> params, antlr4::ParserRuleContext* derivation_tree);

	macro_invo_ptr enter_macro(id_index name, macro_data_ptr label_param_data, std::vector<macro_arg> params);

	void leave_macro();

	void collect_diags() const override; 
};
}
}
}
#endif
