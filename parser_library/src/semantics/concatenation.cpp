#include "concatenation.h"

using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library;

concatenation_point::concatenation_point(const concat_type type)
	: type(type) {}

char_str * concatenation_point::access_str() 
{
	return type == concat_type::STR ? static_cast<char_str*>(this) : nullptr;
}

var_sym * concatenation_point::access_var()
{
	return type == concat_type::VAR ? static_cast<var_sym*>(this) : nullptr;
}

dot * concatenation_point::access_dot()
{
	return type == concat_type::DOT ? static_cast<dot*>(this) : nullptr;
}

equals * concatenation_point::access_equ()
{
	return type == concat_type::EQU ? static_cast<equals*>(this) : nullptr;
}

sublist * concatenation_point::access_sub()
{
	return type == concat_type::SUB ? static_cast<sublist*>(this) : nullptr;
}

char_str::char_str(std::string value) : concatenation_point(concat_type::STR), value(std::move(value)) {}
/*
concat_point_ptr char_str::clone() const
{
	return std::make_unique<char_str>(*this);
}
*/
basic_var_sym::basic_var_sym(id_index name, std::vector<antlr4::ParserRuleContext*> subscript, hlasm_plugin::parser_library::range symbol_range)
	: var_sym(false,std::move(subscript),std::move(symbol_range)), name(name) {}

/*
var_sym::var_sym(const var_sym & variable_symbol)
	: concatenation_point(concat_type::VAR), created(variable_symbol.created), name(variable_symbol.name), symbol_range(variable_symbol.symbol_range), subscript(variable_symbol.subscript)
{
	created_name.insert(created_name.end(), make_clone_iterator(variable_symbol.created_name.begin()), make_clone_iterator(variable_symbol.created_name.end()));
}

var_sym& var_sym::operator=(const var_sym & variable_symbol)
{
	created = variable_symbol.created;
	name = variable_symbol.name;
	symbol_range = variable_symbol.symbol_range;

	created_name.insert(created_name.end(), make_clone_iterator(variable_symbol.created_name.begin()), make_clone_iterator(variable_symbol.created_name.end()));


	for (const auto& expr : variable_symbol.subscript)
		subscript.push_back(expr);

	return *this;
}


var_sym::var_sym() 
	: concatenation_point(concat_type::VAR), created(false),name(context::id_storage::empty_id) {}

*/

/*
concat_point_ptr var_sym::clone() const
{
	return std::make_unique<var_sym>(*this);
}
*/


created_var_sym::created_var_sym(concat_chain created_name, std::vector<antlr4::ParserRuleContext*> subscript, hlasm_plugin::parser_library::range symbol_range)
	: var_sym(true,std::move(subscript),std::move(symbol_range)), created_name(std::move(created_name)) {}

dot::dot()
	:concatenation_point(concat_type::DOT) {}

/*
concat_point_ptr dot::clone() const
{
	return std::make_unique<dot>();
}
concat_point_ptr equals::clone() const
{
	return std::make_unique<equals>();
}
concat_point_ptr sublist::clone() const
{
	concat_chain new_chain;

	new_chain.insert(new_chain.end(), make_clone_iterator(list.begin()), make_clone_iterator(list.end()));

	return std::make_unique<sublist>(std::move(new_chain));
}
*/
equals::equals()
	:concatenation_point(concat_type::EQU) {}



sublist::sublist(concat_chain list) 
	: concatenation_point(concat_type::SUB), list(std::move(list)) {}

void concatenation_point::clear_concat_chain(concat_chain & chain)
{
	size_t offset = 0;
	for (size_t i = 0; i < chain.size(); ++i)
	{
		if (chain[i] && !(chain[i]->type == concat_type::STR && chain[i]->access_str()->value.empty())) //if not empty ptr and not empty string
			chain[offset++] = std::move(chain[i]);
	}

	chain.resize(offset);
}

std::string concatenation_point::to_string(const concat_chain& chain)
{
	std::string ret;
	for (auto& point : chain)
	{
		switch (point->type)
		{
		case concat_type::DOT:
			ret.push_back('.');
			break;
		case concat_type::STR:
			ret.append(point->access_str()->value);
			break;
		case concat_type::VAR:
			ret.push_back('&');
			if(point->access_var()->created)
				ret.append(to_string(point->access_var()->access_created()->created_name));
			else
				ret.append(*point->access_var()->access_basic()->name);
			break;
		default:
			break;
		}
	}
	return ret;
}

bool concatenation_point::contains_var_sym(const concat_chain& chain)
{
	for (const auto& point : chain)
	{
		if (point->type == concat_type::VAR)
			return true;
		else if (point->type == concat_type::SUB)
			if (contains_var_sym(point->access_sub()->list)) return true;
		else
			continue;
	}
	return false;
}

concat_chain concatenation_point::clone(const concat_chain& chain)
{
	concat_chain res, tmp;
	var_sym* var;
	
	for (auto& point : chain)
	{
		switch (point->type)
		{
		case concat_type::DOT:
			res.push_back(std::make_unique<dot>());
			break;
		case concat_type::EQU:
			res.push_back(std::make_unique<equals>());
			break;
		case concat_type::STR:
			res.push_back(std::make_unique<char_str>(point->access_str()->value));
			break;
		case concat_type::SUB:
			tmp = clone(point->access_sub()->list);
			for (auto& tmp_point : tmp) res.push_back(std::move(tmp_point));
		case concat_type::VAR:
			var = point->access_var();
			if (var->created)
				res.push_back(std::make_unique<created_var_sym>(clone(var->access_created()->created_name), var->subscript, var->symbol_range));
			else
				res.push_back(std::make_unique<basic_var_sym>(var->access_basic()->name, var->subscript, var->symbol_range));
		default:
			break;
		}
	}
	return res;
}

basic_var_sym* var_sym::access_basic() 
{
	return created ? nullptr : static_cast<basic_var_sym*>(this);
}

const basic_var_sym* var_sym::access_basic() const
{
	return created ? nullptr : static_cast<const basic_var_sym*>(this);
}

created_var_sym* var_sym::access_created()
{
	return created ? static_cast<created_var_sym*>(this) : nullptr;
}

const created_var_sym* var_sym::access_created() const
{
	return created ? static_cast<const created_var_sym*>(this) : nullptr;
}

var_sym::var_sym(const bool created, std::vector<antlr4::ParserRuleContext*> subscript, const range symbol_range)
	: concatenation_point(concat_type::VAR), created(created), subscript(std::move(subscript)), symbol_range(std::move(symbol_range)) {}
