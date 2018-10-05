#ifndef SEMANTICS_CONCATENATION_H
#define SEMANTICS_CONCATENATION_H

#include <memory>
#include <string>
#include <vector>
#include "semantic_objects.h"
#include "expression.h"
#include "../context/id_storage.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

enum class concat_type { STR, VAR, DOT, SUB };

struct char_str;
struct var_sym;
struct dot;
struct sublist;

struct concatenation_point;
using concat_point_ptr = std::unique_ptr<concatenation_point>;

//helper stuct for character strings that contain variable symbols
//these points of concatenation when formed into array represent character string in a way that is easily concatenated when variable symbols are substituted
struct concatenation_point
{
	char_str* access_str();
	var_sym* access_var();
	dot* access_dot();
	sublist* access_sub();

	virtual concat_type get_type() const = 0;

	virtual ~concatenation_point() = default;
};

//concatenation point representing character string
struct char_str : public concatenation_point
{
	char_str(std::string value);

	std::string value;

	concat_type get_type() const override;
};

//concatenation point representing variable symbol
struct var_sym :public concatenation_point
{
	var_sym(std::string name, std::vector<expr_ptr> subscript, symbol_range range);

	var_sym();

	std::string name;

	symbol_range range;

	std::vector<expr_ptr> subscript;

	concat_type get_type() const override;
};

//concatenation point representing dot
struct dot :public concatenation_point
{
	concat_type get_type() const override;
};

//concatenation point representing macro operand sublist
struct sublist : public concatenation_point
{
	sublist(std::vector<concat_point_ptr> list);

	std::vector<concat_point_ptr> list;

	concat_type get_type() const override;
};

}
}
}
#endif // SEMANTICS_CONCATENATION_H
