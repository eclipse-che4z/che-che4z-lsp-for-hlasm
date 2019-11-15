#include "set_symbol.h"

using namespace hlasm_plugin::parser_library::context;

set_symbol_base::set_symbol_base(id_index name, bool is_scalar, bool is_global, SET_t_enum type)
	: variable_symbol(variable_kind::SET_VAR_KIND, name, is_global), is_scalar(is_scalar), type(type) {}