#ifndef CONTEXT_SYMBOL_ATTRIBUTES_H
#define CONTEXT_SYMBOL_ATTRIBUTES_H

#include <cstdint>

namespace hlasm_plugin {
namespace parser_library {
namespace context {

//enumeration of all data attributes
enum class data_attr_kind { T, L, S, I, K, N, D, O, UNKNOWN };

//structure wrapping attributes of the symbol
//the structure fields are to be constant except undefined fields, their value can be defined later
struct symbol_attributes
{
	using value_t = int32_t;
	using type_attr = uint16_t;
	using len_attr = uint32_t;
	using scale_attr = uint16_t;

	//static field describing undefined states of attributes
	static const type_attr undef_type;
	static const len_attr undef_length;
	static const scale_attr undef_scale;

	//predefined symbol_attributes classes
	static symbol_attributes make_section_attrs();
	static symbol_attributes make_machine_attrs(len_attr);

	//helper function to transform char to enum 
	static data_attr_kind transform_attr(char c);
	static bool needs_ordinary(data_attr_kind attribute);

	symbol_attributes(type_attr type, len_attr length = undef_length, scale_attr scale = undef_scale);
	symbol_attributes();

	//sets whole attributes if undefined
	void set_attributes(const symbol_attributes & attrs);

	type_attr type() const;
	len_attr length() const;
	scale_attr scale() const;

	bool is_defined(data_attr_kind attribute) const;

	value_t get_attribute_value(data_attr_kind attribute) const;

	//sets length if undefined
	void length(len_attr new_length);

private:
	type_attr type_;
	len_attr length_;
	scale_attr scale_;
	bool undefined_;
};

}
}
}
#endif
