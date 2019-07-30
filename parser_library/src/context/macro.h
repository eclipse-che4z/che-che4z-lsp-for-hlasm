#ifndef CONTEXT_MACRO_H
#define CONTEXT_MACRO_H

#include "variable.h"
#include "common_types.h"
#include "hlasm_statement.h"

#include <unordered_map>
#include <string>

namespace hlasm_plugin {
namespace parser_library {
namespace context {

//struct wrapping macro call args
struct macro_arg
{
	id_index id;
	macro_data_ptr data;

	macro_arg(macro_data_ptr data) : id(nullptr), data(std::move(data)) {}
	macro_arg(macro_data_ptr data, id_index name) : id(name), data(std::move(data)) {}
};

struct macro_invocation;
using macro_invo_ptr = std::shared_ptr<macro_invocation>;
using label_storage = std::unordered_map<id_index, sequence_symbol_ptr>;

//class representing macro definition
//contains info about keyword, positional parameters of HLASM mascro as well as derivation tree of the actual code
//has methods call to represent macro instruction call
//serves as prototype for creating macro_invocation objects
class macro_definition
{
	using macro_param_ptr = std::shared_ptr<macro_param_base>;

	std::vector<std::shared_ptr<positional_param>> positional_params_;
	std::unordered_map<id_index, macro_param_ptr> named_params_;
	const id_index label_param_name_;
	
public:
	//identifier of macro
	const id_index id;
	//params of macro
	const std::unordered_map<id_index, macro_param_ptr>& named_params() const;
	//tree representing macro definition
	const statement_block definition;
	//storage of sequence symbols in the macro
	const label_storage labels;
	//location of the macro definition in code
	const location definition_location;
	//initializes macro with its name and params - positional or keyword
	macro_definition(id_index name, id_index label_param_name, std::vector<macro_arg> params, statement_block definition, label_storage labels, location definition_location);

	//returns object with parameters' data set to actual parameters in macro call
	macro_invo_ptr call(macro_data_ptr label_param_data, std::vector<macro_arg> actual_params) const;

	//satifying unordered_map needs 
	bool operator=(const macro_definition& m);

};

//represent macro instruction call 
//contains parameters with set values provided with the call 
struct macro_invocation
{
private:
	std::vector<macro_data_shared_ptr> syslist_;
public:
	//identifier of macro
	const id_index id;
	//params of macro
	const std::unordered_map<id_index, macro_param_ptr> named_params;
	//tree representing macro definition
	const statement_block& definition;
	//storage of sequence symbols in the macro
	const label_storage& labels;
	//location of the macro definition in code
	const location& definition_location;
	//index to definition vector
	int current_statement;
	//stackk get_nest();

	macro_invocation(id_index name, const statement_block& definition, const label_storage& labels, std::unordered_map<id_index, macro_param_ptr> named_params, std::vector<macro_data_shared_ptr> syslist,const location& definition_location);

	//gets syslist value
	const C_t& SYSLIST(size_t idx) const;

	//gets syslist sublisted value
	const C_t& SYSLIST(const std::vector<size_t>& offset) const;
};

}
}
}
#endif
