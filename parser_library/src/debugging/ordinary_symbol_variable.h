#ifndef HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_ORDINARY_SYMBOL_VARIABLE_H
#define HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_ORDINARY_SYMBOL_VARIABLE_H

#include "variable.h"
#include "../context/ordinary_assembly/symbol.h"

namespace hlasm_plugin::parser_library::debugging
{

class ordinary_symbol_variable : public variable
{
public:
	ordinary_symbol_variable(const context::symbol& symbol);


	virtual set_type type() const override;

	virtual bool is_scalar() const override;

	virtual std::vector<variable_ptr> values() const override;
	virtual size_t size() const override;
protected:
	virtual const std::string& get_string_value() const override;
	virtual const std::string& get_string_name() const override;
private:
	const context::symbol& symbol_;
};


}


#endif // !HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_MACRO_PARAM_VARIABLE_H
