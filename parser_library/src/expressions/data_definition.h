#ifndef HLASMPLUGIN_PARSERLIBRARY_EXPRESSIONS_DATA_DEF_H
#define HLASMPLUGIN_PARSERLIBRARY_EXPRESSIONS_DATA_DEF_H

#include <optional>
#include "nominal_value.h"
#include "../context/ordinary_assembly/alignment.h"
#include "../diagnostic_collector.h"
#include "../context/id_storage.h"

namespace hlasm_plugin::parser_library::checking
{
class data_def_type;
}

namespace hlasm_plugin::parser_library::expressions
{

struct data_definition : public diagnosable_op_impl, public context::dependable
{
	
	enum class length_type
	{
		BYTE,
		BIT
	};
	mach_expr_ptr dupl_factor = nullptr;
	char type;
	range type_range;
	char extension = 0;
	range extension_range;
	mach_expr_ptr program_type = nullptr;
	mach_expr_ptr length = nullptr;
	mach_expr_ptr scale = nullptr;
	mach_expr_ptr exponent = nullptr;
	nominal_value_ptr nominal_value = nullptr;

	length_type length_type = length_type::BYTE;

	inline static const char * expr_placeholder = "%";
	inline static const char * nominal_placeholder = " ";
	static data_definition create(std::string format, mach_expr_list exprs, nominal_value_ptr nominal, position begin);

	virtual context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;
	context::dependency_collector get_length_dependencies(context::dependency_solver& solver) const;
	const checking::data_def_type* access_data_def_type() const;
	context::alignment get_alignment() const;
	bool expects_single_symbol() const;
	bool check_single_symbol_ok(const diagnostic_collector& add_diagnostic) const;
	//expects that check_single_symbol_ok returned true
	std::vector<context::id_index> get_single_symbol_names() const;

	void assign_location_counter(context::address loctr_value);

	void collect_diags() const override;
private:
	class parser;
	
};

class data_definition::parser
{
public:
	parser(std::string format, mach_expr_list exprs, nominal_value_ptr nominal, position begin);

	data_definition parse();

	std::optional<int> parse_number();
	size_t get_number_end(size_t begin);
	void update_position(const mach_expression& e);
	void update_position_by_one();
	void parse_duplication_factor();
	void parse_modifier();
	mach_expr_ptr move_next_expression();
	mach_expr_ptr parse_modifier_num_or_expr();
	void assign_expr_to_modifier(char modifier, mach_expr_ptr expr);
private:
	std::string format_;
	mach_expr_list exprs_;
	nominal_value_ptr nominal_;
	position pos_;
	size_t p_;
	size_t exprs_i_;
	data_definition result_;
	std::string remaining_modifiers_ = "PLSE";
	bool nominal_parsed_ = false;
};

}

#endif