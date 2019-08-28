#ifndef CONTEXT_VARIABLES_H
#define CONTEXT_VARIABLES_H

#include "common_types.h"
#include "macro_param_data.h"
#include "id_storage.h"
#include "../../include/shared/range.h"

#include <memory>
#include <map>

namespace hlasm_plugin {
namespace parser_library {
namespace context {

class set_symbol_base;
class macro_param_base;
class variable_symbol;

using var_sym_ptr = std::shared_ptr<variable_symbol>;
using set_sym_ptr = std::shared_ptr<set_symbol_base>;

//base for variable symbols
class variable_symbol
{
public:
	//name of the symbol
	const id_index id;

	//returns kind of variable symbol
	const variable_kind var_kind;

	//casts this to set_symbol_base
	set_symbol_base* access_set_symbol_base();

	//casts this to macro_param
	macro_param_base* access_macro_param_base();

	//N' attribute of the symbol
	virtual A_t number(std::vector<size_t> offset = {}) const = 0;
	//K' attribute of the symbol
	virtual A_t count(std::vector<size_t> offset = {}) const = 0;

	virtual ~variable_symbol() = default;
protected:
	variable_symbol(variable_kind var_kind, id_index name);
};





//*********************set symbols*********************//





template<typename T> class set_symbol;

//base class for set_symbols
class set_symbol_base : public variable_symbol
{
public:
	//describes whether set symbol is scalar
	//when scalar, sublist notation is not allowed
	const bool is_scalar;

	//returns type of set symbol
	const SET_t_enum type;

	//casts this to specialized set symbol
	template <typename T>
	set_symbol<T>* access_set_symbol()
	{
		return (type == object_traits<T>::type_enum) ? static_cast<set_symbol<T>*>(this) : nullptr;
	}

protected:
	set_symbol_base(id_index name, bool is_scalar, SET_t_enum type);
};

//specialized set symbol holding data T (int = A_t, bool = B_t, std::string=C_t)
template<typename T>
class set_symbol : public set_symbol_base
{
	static_assert(object_traits<T>::type_enum != SET_t_enum::UNDEF_TYPE, "Not a SET variable type.");

	//data holding this set_symbol 
	//can be scalar or only array of scallars - no other nesting allowed
	std::map<size_t, T> data;
public:

	set_symbol(id_index name, bool is_scalar) 
		: set_symbol_base(name, is_scalar, object_traits<T>::type_enum) {}

	//gets value from non scalar set symbol
	//if data at idx is not set or it does not exists, default is returned
	const T& get_value(size_t idx) const
	{
		if (is_scalar)
			return object_traits<T>::default_v();

		auto tmp = data.find(idx);
		if (tmp == data.end())
			return object_traits<T>::default_v();
		return tmp->second;
	}

	//gets value from scalar set symbol
	const T& get_value() const
	{
		if (!is_scalar)
			return object_traits<T>::default_v();

		auto tmp = data.find(0);
		if (tmp == data.end())
			return object_traits<T>::default_v();
		return tmp->second;
	}

	//sets value to scalar set symbol
	void set_value(T value)
	{
		data.insert_or_assign(0, std::move(value));
	}

	//sets value to non scalar set symbol
	//any index can be accessed
	void set_value(T value, size_t idx)
	{
		if (is_scalar) 
			data.insert_or_assign(0, std::move(value));
		else
			data.insert_or_assign(idx, std::move(value));
	}

	virtual A_t number(std::vector<size_t> offset = {}) const override
	{
		return (A_t)(is_scalar || data.empty() ? 0 : data.rbegin()->first + 1);
	}

	virtual A_t count(std::vector<size_t> offset = {}) const override;

private:
	const T* get_data(std::vector<size_t> offset = {}) const
	{
		if ((is_scalar && !offset.empty()) || (!is_scalar && offset.size() != 1))
			return nullptr;

		auto tmp_offs = is_scalar ? 0 : offset.front();

		if (data.find(tmp_offs) == data.end())
			return nullptr;

		return &data.at(tmp_offs);
	}
};


template<>
inline A_t set_symbol<A_t>::count(std::vector<size_t> offset) const
{
	auto tmp = get_data(std::move(offset));
	return tmp ? (A_t)std::to_string(*tmp).size() : (A_t)1;
}
template<>
inline A_t set_symbol<B_t>::count(std::vector<size_t> offset) const
{
	return (A_t)1;
}
template<>
inline A_t set_symbol<C_t>::count(std::vector<size_t> offset) const
{
	auto tmp = get_data(std::move(offset));
	return tmp ? (A_t)tmp->size() : (A_t)0;
}





//*********************macro parameters*********************//





class keyword_param;
class positional_param;
class syslist_param;

//class wrapping macro parameters data
class macro_param_base : public variable_symbol
{
public:
	//returns type of macro parameter
	const macro_param_type param_type;

	const keyword_param* access_keyword_param() const;
	const positional_param* access_positional_param() const;
	const syslist_param* access_syslist_param() const;

	//gets value of data where parameter is list of nested data offsets
	virtual const C_t& get_value(std::vector<int> offset) const;
	//gets value of data where parameter is offset to data field
	virtual const C_t& get_value(int idx) const;
	//gets value of whole macro parameter
	virtual const C_t& get_value() const;

	virtual A_t number(std::vector<size_t> offset = {}) const override;
	virtual A_t count(std::vector<size_t> offset = {}) const override;

protected:
	macro_param_base(macro_param_type param_type, id_index name);
	virtual const macro_param_data_component* real_data() const = 0;
};

using macro_param_ptr = std::shared_ptr<macro_param_base>;

//represent macro param with stated position and name, positional param
class keyword_param : public macro_param_base
{
	const macro_data_ptr assigned_data_;
public:
	keyword_param(id_index name, macro_data_shared_ptr default_value, macro_data_ptr assigned_value);

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

//represents macro param with default value, name and no position, keyword param
class syslist_param : public macro_param_base
{
	const macro_data_ptr data_;
public:
	syslist_param(id_index name, macro_data_ptr value);

	virtual const C_t& get_value(std::vector<int> offset) const override;
	virtual const C_t& get_value(int idx) const override;
	virtual const C_t& get_value() const override;

	virtual A_t number(std::vector<size_t> offset = {}) const override;
	virtual A_t count(std::vector<size_t> offset = {}) const override;

protected:
	virtual const macro_param_data_component* real_data() const override;
};




//*********************sequence symbol*********************//
struct sequence_symbol;
struct opencode_sequence_symbol;
struct macro_sequence_symbol;
using sequence_symbol_ptr = std::unique_ptr<sequence_symbol>;

enum class sequence_symbol_kind
{
	OPENCODE,MACRO
};

//structure representing sequence symbol
struct sequence_symbol
{
	id_index name;
	location symbol_location; //TODO for lsp
	sequence_symbol_kind kind;

	const macro_sequence_symbol* access_macro_symbol() const;
	const opencode_sequence_symbol* access_opencode_symbol() const;

	virtual ~sequence_symbol() = default;

protected:
	sequence_symbol(id_index name, const sequence_symbol_kind kind, location symbol_location);
};

struct opencode_sequence_symbol : public sequence_symbol
{
	struct opencode_position {
		opencode_position(size_t file_line=0, size_t file_offset=0) :file_line(file_line), file_offset(file_offset) {}
		size_t file_line; size_t file_offset; 
		bool operator==(const opencode_position& oth) const { return file_line == oth.file_line && file_offset == oth.file_offset; }
	};

	struct copy_frame { 
		id_index copy_member; size_t statement_offset;
		bool operator==(const copy_frame& oth) const { return copy_member == oth.copy_member && statement_offset == oth.statement_offset; }
	};

	opencode_position statement_position;
	std::vector<copy_frame> copy_stack;

	opencode_sequence_symbol(id_index name, location symbol_location, opencode_position statement_position, std::vector<copy_frame> copy_stack = {});

	bool operator==(const opencode_sequence_symbol& oth) const;
};

struct macro_sequence_symbol : public sequence_symbol
{
	size_t statement_offset;

	macro_sequence_symbol(id_index name, location symbol_location, size_t statement_offset);
};

}
}
}
#endif
