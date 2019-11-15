#ifndef CONTEXT_SYSTEM_VARIABLE_H
#define CONTEXT_SYSTEM_VARIABLE_H

#include "macro_param.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

class system_variable;
using sys_sym_ptr = std::shared_ptr<system_variable>;

//base for variable symbols
class system_variable : public macro_param_base
{
	const macro_data_ptr data_;
public:
	system_variable(id_index name, macro_data_ptr value, bool is_global);

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

	virtual size_t size(std::vector<size_t> offset = {}) const override;

protected:
	virtual const macro_param_data_component* real_data() const override;
};

}
}
}

#endif
