#ifndef CONTEXT_SYMBOL_H
#define CONTEXT_SYMBOL_H

#include <variant>
#include <limits>

#include "id_storage.h"
#include "symbol_attributes.h"
#include "address.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

//defines kind of symbol, absolute or relocatable
enum class symbol_kind
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

	symbol_kind value_kind() const;

private:
	std::variant<std::monostate, abs_value_t, reloc_value_t> value_;
};

//class representing ordinary symbol
//the value and attributes fields have the same semantics as described in symbol_attributes
class symbol
{
public:
	symbol(id_index name, symbol_value value, symbol_attributes attributes = symbol_attributes());
	symbol();

	const symbol_value& value() const;

	symbol_kind kind() const;

	void set_value(symbol_value value);

	const id_index name;
private:
	symbol_value value_;
	symbol_attributes attributes_;
};

}
}
}
#endif
