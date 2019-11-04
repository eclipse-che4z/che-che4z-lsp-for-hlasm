#ifndef CONTEXT_SYMBOL_H
#define CONTEXT_SYMBOL_H

#include <variant>
#include <limits>

#include "symbol_attributes.h"
#include "address.h"
#include "shared/range.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

//defines kind of symbol value, absolute or relocatable or undefined
enum class symbol_value_kind
{
	UNDEF = 0, ABS = 1, RELOC = 2
};

//structure holding value of a symbol
struct symbol_value
{
	using abs_value_t = int32_t;
	using reloc_value_t = address;

	symbol_value(abs_value_t value);
	symbol_value(reloc_value_t value);
	symbol_value();

	symbol_value operator+(const symbol_value& value) const;
	symbol_value operator-(const symbol_value& value) const;
	symbol_value operator*(const symbol_value& value) const;
	symbol_value operator/(const symbol_value& value) const;
	symbol_value operator-() const;

	symbol_value& operator=(const symbol_value& value);

	const abs_value_t& get_abs() const;
	const reloc_value_t& get_reloc() const;

	symbol_value_kind value_kind() const;

private:
	std::variant<std::monostate, abs_value_t, reloc_value_t> value_;
};

//class representing ordinary symbol
//the value and attributes fields have the same semantics as described in symbol_attributes
class symbol
{
public:
	symbol(id_index name, symbol_value value, symbol_attributes attributes, location symbol_location);

	const symbol_value& value() const;
	const symbol_attributes& attributes() const;

	symbol_value_kind kind() const;

	void set_value(symbol_value value);
	void set_length(symbol_attributes::len_attr value);
	void set_scale(symbol_attributes::scale_attr value);

	const id_index name;
	const location symbol_location;
private:
	symbol_value value_;
	symbol_attributes attributes_;
};

}
}
}
#endif
