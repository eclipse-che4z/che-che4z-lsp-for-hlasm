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

#ifndef HLASMPLUGIN_HLASMPARSERLIBARY_PARSER_TEST_H
#define HLASMPLUGIN_HLASMPARSERLIBARY_PARSER_TEST_H

#include "common_testing.h"
#include "../src/expressions/visitors/expression_evaluator.h"
#include "../src/parser_tools.h"

//tests for 
//parsing various files
//parsing CA expressions

class library_test : public testing::Test
{
public:
	virtual void setup(std::string param)
	{
		input = get_content("test/library/input/" + param + ".in");
		holder = std::make_unique<analyzer>(input);
	}
protected:
	std::unique_ptr<analyzer> holder;
	std::string input;
};

TEST_F(library_test, expression_test)
{
	std::string tcase = "expression_test";

	setup(tcase);
	auto tree = holder->parser().expr_test();
	context_manager m(holder->context());
	empty_attribute_provider attr_mock;
	auto v = std::make_unique<expression_evaluator>(evaluation_context{ holder->context(),attr_mock, empty_parse_lib_provider::instance });
	auto id = holder->context().ids().add("TEST_VAR");
	holder->context().create_local_variable<int>(id, true);
	holder->context().get_var_sym(id)->access_set_symbol_base()->access_set_symbol<A_t>()->set_value(11);

	auto res = v->visit(tree).as<std::string>();
	res.erase(std::remove(res.begin(), res.end(), '\r'), res.end());
	ASSERT_EQ(res, get_content("test/library/output/" + tcase + ".output"));

	//no errors found while parsing
	ASSERT_EQ(holder->parser().getNumberOfSyntaxErrors(), size_t_zero);
}

//3 instruction statements and 3 EOLLN
TEST_F(library_test, simple)
{
	std::string tcase = "simple";

	setup(tcase);

	//compare tokens with output file

	holder->analyze();
	//no errors found while parsing
	ASSERT_EQ(holder->parser().getNumberOfSyntaxErrors(), size_t_zero);
}

//5 instruction statements that have 1,1,1,2 and 4 operands respectively
TEST_F(library_test, operand)
{
	std::string tcase = "operand";

	setup(tcase);

	//compare tokens with output file

	holder->analyze();
	//no errors found while parsing
	ASSERT_EQ(holder->parser().getNumberOfSyntaxErrors(), size_t_zero);

}

//3 alternative forms of instructions
TEST_F(library_test, continuation)
{
	std::string tcase = "continuation";

	setup(tcase);

	holder->analyze();
	//no errors found while parsing
	ASSERT_EQ(holder->parser().getNumberOfSyntaxErrors(), size_t_zero);
}
#define CORRECTNESS_TEST
#ifdef CORRECTNESS_TEST
//simply parse correctly
TEST_F(library_test, correctness)
{
	std::string tcase = "correctness";

	setup(tcase);

	//compare tokens with output file
	//ASSERT_EQ(token_string, get_content("test/library/output/tokens/" + tcase + ".output"));

	holder->analyze();
	//no errors found while parsing
	ASSERT_EQ(holder->parser().getNumberOfSyntaxErrors(), size_t_zero);
}
#endif

//finding 3 variable symbols in model statement
TEST_F(library_test, model_statement)
{
	std::string tcase = "model_statement";

	setup(tcase);


	holder->analyze();
	//no errors found while parsing
	ASSERT_EQ(holder->parser().getNumberOfSyntaxErrors(), size_t_zero);
}

//simply parse correctly
TEST_F(library_test, comment)
{
	std::string tcase = "comment";

	setup(tcase);

	//compare tokens with output file

	holder->analyze();
	//no errors found while parsing
	ASSERT_EQ(holder->parser().getNumberOfSyntaxErrors(), size_t_zero);
}

//simply parse correctly
TEST_F(library_test, empty_string)
{
	std::string es_input = " LR ''";

	analyzer a(es_input);
	a.analyze();
	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)2);
}

TEST_F(library_test, long_macro)
{
	std::string tcase = "long_macro";

	setup(tcase);

	//compare tokens with output file

	holder->analyze();
	//no errors found while parsing
	ASSERT_EQ(holder->parser().getNumberOfSyntaxErrors(), size_t_zero);
}

TEST_F(library_test, process_statement)
{
	std::string tcase = "process";

	setup(tcase);

	//compare tokens with output file

	holder->analyze();
	//no errors found while parsing
	ASSERT_EQ(holder->parser().getNumberOfSyntaxErrors(), size_t_zero);
}

TEST_F(library_test, macro_model)
{
	std::string tcase = "macro_model";

	setup(tcase);

	//compare tokens with output file

	holder->analyze();
	//no errors found while parsing
	ASSERT_EQ(holder->parser().getNumberOfSyntaxErrors(), size_t_zero);
}

#endif // !HLASMPLUGIN_HLASMPARSERLIBARY_PARSER_TEST_H
