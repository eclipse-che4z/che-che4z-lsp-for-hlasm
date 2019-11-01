#ifndef HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_ORD_SYM_ATTRIBUTE_VARIABLE_H
#define HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_ORD_SYM_ATTRIBUTE_VARIABLE_H

#include "variable.h"
#include "../context/ordinary_assembly/symbol.h"

namespace hlasm_plugin::parser_library::debugging
{

class attribute_variable : public variable
{
public:
	attribute_variable(std::string name, std::string value);

	virtual set_type type() const override;

	virtual bool is_scalar() const override;

	virtual std::vector<variable_ptr> values() const override;
	virtual size_t size() const override;
protected:
	virtual const std::string& get_string_value() const override;
	virtual const std::string& get_string_name() const override;
};


}


#endif // !HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_MACRO_PARAM_VARIABLE_H
