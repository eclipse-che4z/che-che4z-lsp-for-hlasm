#ifndef CONTEXT_SYMBOL_ATTRIBUTES_H
#define CONTEXT_SYMBOL_ATTRIBUTES_H

#include <cstdint>

namespace hlasm_plugin {
namespace parser_library {
namespace context {

//structure wrapping attributes of the symbol
//the structure is mainly immutable except undefined fields, their value can be defined later
struct symbol_attributes
{
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

	symbol_attributes(type_attr type, len_attr length = undef_length, scale_attr scale = undef_scale);
	symbol_attributes();

	//sets whole attributes if undefined
	void set_attributes(const symbol_attributes & attrs);

	type_attr type() const;
	len_attr length() const;
	scale_attr scale() const;

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
