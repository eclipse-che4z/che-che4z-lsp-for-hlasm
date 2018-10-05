#include "concatenation.h"

using namespace hlasm_plugin::parser_library::semantics;

char_str * hlasm_plugin::parser_library::semantics::concatenation_point::access_str() { return dynamic_cast<char_str*>(this); }

var_sym * hlasm_plugin::parser_library::semantics::concatenation_point::access_var() { return dynamic_cast<var_sym*>(this); }

dot * hlasm_plugin::parser_library::semantics::concatenation_point::access_dot() { return dynamic_cast<dot*>(this); }

sublist * hlasm_plugin::parser_library::semantics::concatenation_point::access_sub() { return dynamic_cast<sublist*>(this); }

hlasm_plugin::parser_library::semantics::char_str::char_str(std::string value) :value(std::move(value)) {}

concat_type hlasm_plugin::parser_library::semantics::char_str::get_type() const { return concat_type::STR; }

hlasm_plugin::parser_library::semantics::var_sym::var_sym(std::string name, std::vector<expr_ptr> subscript, symbol_range range)
	: name(std::move(name)), subscript(std::move(subscript)), range(range) {}

hlasm_plugin::parser_library::semantics::var_sym::var_sym() {}

concat_type hlasm_plugin::parser_library::semantics::var_sym::get_type() const { return concat_type::VAR; }

concat_type hlasm_plugin::parser_library::semantics::dot::get_type() const { return concat_type::DOT; }

hlasm_plugin::parser_library::semantics::sublist::sublist(std::vector<concat_point_ptr> list) : list(std::move(list)) {}

concat_type hlasm_plugin::parser_library::semantics::sublist::get_type() const { return concat_type::SUB; }
