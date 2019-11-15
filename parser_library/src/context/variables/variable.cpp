#include "variable.h"
#include "set_symbol.h"
#include "macro_param.h"

using namespace std;

namespace hlasm_plugin::parser_library::context
{

set_symbol_base * variable_symbol::access_set_symbol_base()
{
	return dynamic_cast<set_symbol_base*>(this);
}

const set_symbol_base* variable_symbol::access_set_symbol_base() const
{
	return dynamic_cast<const set_symbol_base*>(this);
}

macro_param_base * variable_symbol::access_macro_param_base()
{
	return dynamic_cast<macro_param_base*>(this);
}

const macro_param_base* variable_symbol::access_macro_param_base() const
{
	return dynamic_cast<const macro_param_base*>(this);
}

variable_symbol::variable_symbol(variable_kind var_kind, id_index name, bool is_global)
	:id(name), is_global(is_global), var_kind(var_kind) {}




}
