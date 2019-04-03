#ifndef SEMANTICS_CONCATENATION_H
#define SEMANTICS_CONCATENATION_H

#include <memory>
#include <string>
#include <vector>
#include <iterator>
#include "semantic_objects.h"
#include "expression.h"
#include "../context/id_storage.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

enum class concat_type { STR, VAR, DOT, SUB, EQUALS };

struct char_str;
struct var_sym;
struct dot;
struct equals;
struct sublist;

struct concatenation_point;
using concat_point_ptr = std::unique_ptr<concatenation_point>;
using concat_chain = std::vector<concat_point_ptr>;

//cleans concat_chains of empty strings and badly parsed operands 
void clear_concat_chain(concat_chain& conc_list);



template <typename it_T>
struct clone_iterator : public std::iterator<std::input_iterator_tag,typename it_T::value_type, typename it_T::difference_type, typename it_T::pointer, typename it_T::reference>
{
	clone_iterator(it_T iterator) :iterator_(std::move(iterator)) {}

	clone_iterator& operator++() { iterator_++; return *this;}
	bool operator==(const clone_iterator& rhs) const { return iterator_ == rhs.iterator_;}
	bool operator!=(const clone_iterator& rhs) const  { return iterator_ != rhs.iterator_; }
	auto operator*() const { return (*iterator_)->clone();}

private:
	it_T iterator_;
};

template <typename it_T>
clone_iterator<it_T> make_clone_iterator(it_T iterator) { return clone_iterator<it_T>(std::move(iterator)); };

//helper stuct for character strings that contain variable symbols
//these points of concatenation when formed into array represent character string in a way that is easily concatenated when variable symbols are substituted
struct concatenation_point
{
	char_str* access_str();
	var_sym* access_var();
	dot* access_dot();
	equals* access_equ();
	sublist* access_sub();

	virtual concat_type get_type() const = 0;

	virtual concat_point_ptr clone() const = 0;

	virtual ~concatenation_point() = default;
};

//concatenation point representing character string
struct char_str : public concatenation_point
{
	char_str(std::string value);

	std::string value;

	concat_type get_type() const override;

	concat_point_ptr clone() const override;
};

//concatenation point representing variable symbol
struct var_sym :public concatenation_point
{
	var_sym(std::string name, std::vector<antlr4::ParserRuleContext*> subscript, symbol_range range);

	var_sym(concat_chain created_name, std::vector<antlr4::ParserRuleContext*> subscript, symbol_range range);

	var_sym(const var_sym& variable_symbol);

	var_sym& operator=(const var_sym& variable_symbol);

	var_sym(var_sym&&) = default;

	var_sym& operator=(var_sym&&) = default;

	var_sym();

	bool created;

	std::string name;

	concat_chain created_name;

	symbol_range range;

	std::vector<antlr4::ParserRuleContext*> subscript;

	concat_type get_type() const override;

	concat_point_ptr clone() const override;
};

//concatenation point representing dot
struct dot :public concatenation_point
{
	concat_type get_type() const override;

	concat_point_ptr clone() const override;
};

//concatenation point representing equals sign
struct equals :public concatenation_point
{
	concat_type get_type() const override;

	concat_point_ptr clone() const override;
};

//concatenation point representing macro operand sublist
struct sublist : public concatenation_point
{
	sublist(concat_chain list);

	concat_chain list;

	concat_type get_type() const override;

	concat_point_ptr clone() const override;
};

}
}
}
#endif // SEMANTICS_CONCATENATION_H
