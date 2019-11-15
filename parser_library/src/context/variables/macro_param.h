#ifndef CONTEXT_MACRO_PARAM_H
#define CONTEXT_MACRO_PARAM_H

#include "variable.h"
#include "../macro_param_data.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

class keyword_param;
class positional_param;
class system_variable;

//class wrapping macro parameters data
class macro_param_base : public variable_symbol
{
public:
	//returns type of macro parameter
	const macro_param_type param_type;

	const keyword_param* access_keyword_param() const;
	const positional_param* access_positional_param() const;
	const system_variable* access_system_variable() const;

	//gets value of data where parameter is list of nested data offsets
	virtual const C_t& get_value(const std::vector<size_t>& offset) const;
	//gets value of data where parameter is offset to data field
	virtual const C_t& get_value(size_t idx) const;
	//gets value of whole macro parameter
	virtual const C_t& get_value() const;

	//gets param struct
	virtual const macro_param_data_component* get_data(const std::vector<size_t>& offset) const;

	//N' attribute of the symbol
	virtual A_t number(std::vector<size_t> offset = {}) const override;
	//K' attribute of the symbol
	virtual A_t count(std::vector<size_t> offset = {}) const override;

	virtual size_t size(std::vector<size_t> offset = {}) const;
protected:
	macro_param_base(macro_param_type param_type, id_index name, bool is_global);
	virtual const macro_param_data_component* real_data() const = 0;
};

using macro_param_ptr = std::shared_ptr<macro_param_base>;

//represent macro param with stated position and name, positional param
class keyword_param : public macro_param_base
{
	const macro_data_ptr assigned_data_;
public:
	keyword_param(id_index name, macro_data_shared_ptr default_value, macro_data_ptr assigned_value);

	//default macro keyword parameter data
	const macro_data_shared_ptr default_data;
protected:
	virtual const macro_param_data_component* real_data() const override;
};

//represents macro param with default value, name and no position, keyword param
class positional_param : public macro_param_base
{
	const macro_param_data_component& data_;
public:
	const size_t position;

	positional_param(id_index name, size_t position, const macro_param_data_component& assigned_value);

protected:
	virtual const macro_param_data_component* real_data() const override;
};

}
}
}

#endif
