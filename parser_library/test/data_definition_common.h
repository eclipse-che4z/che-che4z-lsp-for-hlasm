/*
 * Copyright (c) 2019 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Broadcom, Inc. - initial API and implementation
 */

#ifndef HLASMPLUGIN_PARSERLIBRARY_TEST_DATA_DEFINITION_COMMON_H
#define HLASMPLUGIN_PARSERLIBRARY_TEST_DATA_DEFINITION_COMMON_H

#include "../src/checking/data_definition/data_def_types.h"
#include "../src/checking/asm_instr_check.h"
#include "../src/analyzer.h"
#include "../src/diagnostic_collector.h"

using namespace hlasm_plugin::parser_library::checking;
using namespace hlasm_plugin::parser_library;

#define ADD_DIAG(col) diagnostic_collector(&col)

class diag_collector : public diagnosable_ctx
{
public:
	diag_collector() : diagnosable_ctx(ctx) {}
	// Inherited via diagnosable_ctx
	virtual void collect_diags() const override {};

	context::hlasm_context ctx;
};

data_definition_operand setup_data_def_op(char type, char extension)
{
	data_definition_operand op;
	op.type = data_def_field(true, type, range());
	op.extension = data_def_field(extension != '\0', extension, range());
	return op;
}
data_definition_operand setup_data_def_op(char type, char extension, int32_t length)
{
	data_definition_operand op = setup_data_def_op(type, extension);
	op.length.present = true;
	op.length.value = length;
	op.length.len_type = data_def_length_t::BYTE;
	return op;
}

data_definition_operand setup_data_def_op(char type, char extension, std::variant<std::string, nominal_value_expressions> nominal)
{
	data_definition_operand op = setup_data_def_op(type, extension);
	op.nominal_value.present = true;
	op.nominal_value.value = std::move(nominal);
	return op;
}

data_definition_operand setup_data_def_op(char type, char extension, std::variant<std::string, nominal_value_expressions> nominal, int32_t length)
{
	data_definition_operand op = setup_data_def_op(type, extension, nominal);
	op.length.present = true;
	op.length.value = length;
	op.length.len_type = data_def_length_t::BYTE;
	return op;
}

nominal_value_expressions setup_expressions() {
	return nominal_value_expressions();
}

template<typename ... values>
nominal_value_expressions setup_expressions(int32_t value, expr_type type, const values& ... rest)
{
	nominal_value_expressions op = setup_expressions(rest...);

	op.push_back(data_def_expr{ value, type, range() });
	return op;
}

template<typename ... values>
data_definition_operand setup_data_def_expr_op(char type, char extension, const values& ... expr_values_and_types)
{
	nominal_value_expressions op = setup_expressions(expr_values_and_types...);

	return setup_data_def_op(type, extension, op);
}

template<typename ... values>
data_definition_operand setup_data_def_expr_op_length(char type, char extension, int32_t length, const values& ... expr_values_and_types)
{
	nominal_value_expressions op = setup_expressions(expr_values_and_types...);

	return setup_data_def_op(type, extension, op, length);
}



data_def_address make_data_def_address(int32_t base, int32_t disp)
{
	return data_def_address{ data_def_field(true, base, range()), data_def_field(true, disp, range()) };
}

nominal_value_expressions setup_addresses() {
	return nominal_value_expressions();
}

template<typename ... values>
nominal_value_expressions setup_addresses(int32_t base, int32_t disp, const values& ... rest)
{
	nominal_value_expressions op = setup_addresses(rest...);

	op.push_back(make_data_def_address(base, disp));
	return op;
}

template<typename ... values>
data_definition_operand setup_data_def_addr_op(char type, char extension, const values& ... expr_addresses)
{
	nominal_value_expressions op = setup_addresses(expr_addresses...);

	return setup_data_def_op(type, extension, op);
}

#endif
