#ifndef HLASMPLUGIN_PARSERLIBRARY_CHECKING_DATA_DEF_FIELDS_H
#define HLASMPLUGIN_PARSERLIBRARY_CHECKING_DATA_DEF_FIELDS_H

namespace hlasm_plugin::parser_library::checking
{

//Represents data def modifiers, type, extension and duplication factor
template<typename T>
struct data_def_field
{
	data_def_field()
		: present(false), value(), rng() {}
	data_def_field(T value)
		: present(true), value(std::move(value)), rng() {}
	data_def_field(bool present, T value, range rng)
		: present(present), value(std::move(value)), rng(rng) {}
	bool present;
	T value;
	range rng;
};
//Specifies whether expression was absolute, relocatable or complex relocatable.
enum class expr_type
{
	ABS,
	RELOC,
	COMPLEX
};
//Represents an expression in nominal value of data definition operand.
struct data_def_expr
{
	int32_t value;
	expr_type ex_kind;
	range rng;
	//When ignored is true, the expression should be ignored by checker.
	bool ignored = false;
};
//Represents the length modifier, adds length type.
struct data_def_length : data_def_field<int32_t>
{
	enum length_type
	{
		BYTE,
		BIT
	};

	data_def_length() : len_type(length_type::BYTE) {}
	data_def_length(data_def_field<int32_t> field) : data_def_field<int32_t>(std::move(field)), len_type(length_type::BYTE) {}

	length_type len_type;
};
//Represents values of data definition operand written in form D(B).
struct data_def_address
{
	data_def_field<int32_t> base;
	data_def_field<int32_t> displacement;
	bool ignored = false;
};

using expr_or_address = std::variant<data_def_expr, data_def_address>;
using nominal_value_expressions = std::vector<expr_or_address>;
using nominal_value_t = data_def_field < std::variant<std::string, nominal_value_expressions> >;

}

#endif // !HLASMPLUGIN_PARSERLIBRARY_CHECKING_DATA_DEF_FIELDS_H
