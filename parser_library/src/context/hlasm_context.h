#ifndef CONTEXT_HLASM_CONTEXT_H
#define CONTEXT_HLASM_CONTEXT_H

#include "id_storage.h"
#include "macro.h"
#include <memory>
#include <stack>
#include "code_scope.h"

namespace hlasm_plugin{
namespace parser_library{
namespace context{

//class helping to perform semantic analysis of hlasm source code
//wraps all classes and structures needed by semantic analysis (like variable symbol tables, opsyn tables...) in one place
//contains methods that store gathered information from semantic analysis helping it to correctly evaluate parsed code 
class hlasm_context
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

	inline code_scope* curr_scope();

public:
	hlasm_context();

	//storage for identifiers
	id_storage ids;

	//return map of global set vars
	const code_scope::set_sym_storage& globals() const;

	//return variable symbol in current scope
	//returns empty shared_ptr if there is none
	var_sym_ptr get_var_sym(id_index name);

	//TODO get sequence symbol
	//...

	//adds opsyn mnemotechnic
	void add_mnemonic(id_index target, id_index op_code);

	//removes opsyn mnenotechnic
	void remove_mnemonic(id_index target);

	//gets mnemotechnic possibly changed by opsyn
	//returns false class if there is no mnemotechnic
	//returns index to default string ("") if opcode was deleted by opsyn
	//returns anything else if opcode was changed by opsyn
	id_index get_mnemonic(id_index op_code) const;

	//creates specified global set symbol
	template <typename T>
	set_sym_ptr create_global_variable(id_index id, bool is_scalar)
	{
		//TODO error handling
		//if there is symbolic param with same name
		//if there is set symbol with same name

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
};
}
}
}
#endif 
