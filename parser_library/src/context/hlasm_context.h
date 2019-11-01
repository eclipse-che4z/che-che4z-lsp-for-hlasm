#ifndef CONTEXT_HLASM_CONTEXT_H
#define CONTEXT_HLASM_CONTEXT_H

#include <memory>
#include <deque>
#include <vector>
#include <set>

#include "lsp_context.h"
#include "code_scope.h"
#include "id_storage.h"
#include "macro.h"
#include "ordinary_assembly/ordinary_assembly_context.h"
#include "instruction.h"
#include "processing_context.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

class hlasm_context;
using ctx_ptr = std::unique_ptr<hlasm_context>;

//class helping to perform semantic analysis of hlasm source code
//wraps all classes and structures needed by semantic analysis (like variable symbol tables, opsyn tables...) in one place
//contains methods that store gathered information from semantic analysis helping it to correctly evaluate parsed code 
class hlasm_context
{
	using macro_storage = std::unordered_map<id_index, std::unique_ptr<macro_definition>>;
	using literal_map = std::unordered_map<id_index, id_index>;
	using copy_member_storage = std::unordered_map<id_index, copy_member>;
	using instruction_storage = std::unordered_map<id_index, instruction::instruction_array>;

	//storage of global variables
	code_scope::set_sym_storage globals_;
	//storage of defined macros
	macro_storage macros_;
	//storage of copy members
	copy_member_storage copy_members_;
	//map of OPSYN mnemonics
	literal_map opcode_mnemo_;
	//storage of identifiers
	id_storage ids_;

	//stack of nested scopes
	std::deque<code_scope> scope_stack_;
	code_scope* curr_scope();
	const code_scope* curr_scope() const;
	//stack of statement processings
	std::vector<processing_context> proc_stack_;
	//stack of processed source files
	std::vector<source_context> source_stack_;
	//stack of nested copy member invocations
	std::vector<copy_member_invocation> copy_stack_;

	//all files processes via macro or copy member invocation
	std::set<std::string> visited_files_;

	//map of all instruction in HLASM
	const instruction_storage instruction_map_;
	instruction_storage init_instruction_map();

	//value of system variable SYSNDX
	size_t SYSNDX_;
	void add_system_vars_to_scope();
public:

	hlasm_context(std::string file_name = "");

	//gets name of file where is open-code located
	const std::string& opencode_file_name() const;
	//accesses visited files
	const std::set<std::string>& get_visited_files();

	//gets current source
	const source_context& current_source() const;
	//sets current scope according to the snapshot
	void apply_source_snapshot(source_snapshot snapshot);
	//sets current source position
	void set_source_position(position pos);
	//sets current source file indices
	void set_source_indices(size_t begin_index, size_t end_index);

	//pushes new kind of statement processing
	void push_statement_processing(const processing::processing_kind kind);
	//pushes new kind of statement processing as well as new source
	void push_statement_processing(const processing::processing_kind kind, std::string file_name);
	//pops statement processing
	void pop_statement_processing();

	//gets stack of locations of all currently processed files
	processing_stack_t processing_stack() const;
	//gets macro nest
	const std::deque<code_scope>& scope_stack() const;
	//gets copy nest of current statement processing
	std::vector<copy_member_invocation>& current_copy_stack();
	//gets names of whole copy nest
	std::vector<id_index> whole_copy_stack() const;

	const code_scope & current_scope() const;

	//index storage
	id_storage& ids();

	//map of instructions
	const instruction_storage& instruction_map() const;

	//field that accessed ordinary assembly context
	ordinary_assembly_context ord_ctx;
	//field that accessed LSP context
	lsp_ctx_ptr lsp_ctx;

	//return map of global set vars
	const code_scope::set_sym_storage& globals() const;

	//return variable symbol in current scope
	//returns empty shared_ptr if there is none in the current scope
	var_sym_ptr get_var_sym(id_index name);

	//registers sequence symbol
	void add_sequence_symbol(sequence_symbol_ptr seq_sym);
	//return sequence symbol in current scope
	//returns nullptr if there is none in the current scope
	const sequence_symbol* get_sequence_symbol(id_index name) const;

	void set_branch_counter(A_t value);
	A_t get_branch_counter() const;
	void decrement_branch_counter();

	//adds opsyn mnemonic
	void add_mnemonic(id_index mnemo,id_index op_code);
	//removes opsyn mnemonic
	void remove_mnemonic(id_index mnemo);

	//gets target of possible mnemonic changed by opsyn
	//returns nullptr if there is no mnemonic
	//returns index to default string ("") if opcode was deleted by opsyn
	//returns anything else if opcode was changed by opsyn
	id_index get_mnemonic_opcode(id_index mnemo) const;

	//get data attribute value of variable symbol
	SET_t get_data_attribute(data_attr_kind attribute, var_sym_ptr var_symbol, std::vector<size_t> offset = {});
	//get data attribute value of ordinary symbol
	SET_t get_data_attribute(data_attr_kind attribute, id_index symbol);

	C_t get_type_attr(var_sym_ptr var_symbol, const std::vector<size_t>& offset);
	C_t get_opcode_attr(id_index symbol);

	//gets macro storage
	const macro_storage& macros() const;
	//checks whether processing is currently in macro
	bool is_in_macro() const;
	//returns macro we are currently in or empty shared_ptr if in open code
	macro_invo_ptr this_macro() const;
	//registers new macro
	const macro_definition& add_macro(
		id_index name,
		id_index label_param_name, std::vector<macro_arg> params,
		statement_block definition, copy_nest_storage copy_nests, label_storage labels,
		location definition_location);
	//enters a macro with actual params
	macro_invo_ptr enter_macro(id_index name, macro_data_ptr label_param_data, std::vector<macro_arg> params);
	//leaves current macro
	void leave_macro();

	//gets copy member storage
	const copy_member_storage& copy_members();
	//registers new copy member
	void add_copy_member(id_index member, statement_block definition, location definition_location);
	//enters a copy member
	void enter_copy_member(id_index member);
	//leaves current copy member
	void leave_copy_member();

	//creates specified global set symbol
	template <typename T>
	set_sym_ptr create_global_variable(id_index id,bool is_scalar)
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

		set_sym_ptr val(std::make_shared<set_symbol<T>>(id, is_scalar, true));

		globals_.insert({ id,val });
		curr_scope()->variables.insert({ id,val });

		return val;
	}

	//creates specified local set symbol
	template <typename T>
	set_sym_ptr create_local_variable(id_index id,bool is_scalar)
	{
		auto tmp = curr_scope()->variables.find(id);
		if (tmp != curr_scope()->variables.end())
			return tmp->second;


		set_sym_ptr val(std::make_shared<set_symbol<T>>(id, is_scalar, false));

		curr_scope()->variables.insert({ id,val });

		return val;
	}

};

}
}
}
#endif
