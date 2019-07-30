#include "../src/analyzer.h"

void expect_no_errors(const std::string & text)
{
	using namespace hlasm_plugin::parser_library;
	analyzer a(text);
	a.analyze();
	a.collect_diags();
	EXPECT_EQ(a.diags().size(), (size_t)0);
}

void expect_errors(const std::string& text)
{
	using namespace hlasm_plugin::parser_library;
	analyzer a(text);
	a.analyze();
	a.collect_diags();
	EXPECT_GT(a.diags().size(), (size_t)0);
}

TEST(data_definition_grammar, modifiers)
{
	expect_no_errors(R"( DC 10FDP(123)L(2*3)S(2*4)E(-12*2)'2.25'
 DC 10FDP(123)L2S(2*4)E(-12*2)'2.25'
 DC 10FDP(123)L(2*3)S6E(-12*2)'2.25'
 DC 10FDP(123)L.(2*3)S6E(-12*2)'2.25'
 DC 10FDP(123)L(2*3)S(2*4)E12'2.25'
 DC 10FDP(123)L(2*3)S(2*4)E-12'2.25'
 DC 10FDP(123)L(2*3)S6E0'2.25'
 DC 10FDP(123)L.(2*3)S6E0'2.25'
 DC 10FDP(123)L3S(2*4)E12'2.25'
 DC 10FDP(123)L1S30E(-12*2)'2.25'
 DC 10FDP(123)L1S-30E(-12*2)'2.25'
 DC 10FDP(123)L.1S30E(-12*2)'2.25'
 DC 10FDP(123)L1S30E40'2.25'
 DC 10FDP(123)L1S-30E-40'2.25'
 DC 10FDP(123)L.1S30E40'2.25'
 DC 10FDP(123)L.1S-30E-40'2.25'

 DC (1*8)FDP(123)L2S(2*4)E(-12*2)'2.25'
 DC (1*8)FDP(123)L(2*3)S6E(-12*2)'2.25'
 DC (1*8)FDP(123)L(2*3)S(2*4)E12'2.25'
 DC (1*8)FDP(123)L(2*3)S6E0'2.25'
 DC (1*8)FDP(123)L3S(2*4)E12'2.25'
 DC (1*8)FDP(123)L1S30E(-12*2)'2.25'
 DC (1*8)FDP(123)L1S30E40'2.25'
	
 DC 10FDL(2*3)S(2*4)E(-12*2)'2.25'
 DC 10FDL2S(2*4)E(-12*2)'2.25'
 DC 10FDL(2*3)S6E(-12*2)'2.25'
 DC 10FDL(2*3)S(2*4)E12'2.25'
 DC 10FDL(2*3)S6E0'2.25'
 DC 10FDL3S(2*4)E12'2.25'
 DC 10FDL1S30E(-12*2)'2.25'
 DC 10FDL1S30E40'2.25'

 DC (1*8)FDL(2*3)S(2*4)E(-12*2)'2.25'
 DC (1*8)FDL2S(2*4)E(-12*2)'2.25'
 DC (1*8)FDL(2*3)S6E(-12*2)'2.25'
 DC (1*8)FDL(2*3)S(2*4)E12'2.25'
 DC (1*8)FDL(2*3)S6E0'2.25'
 DC (1*8)FDL3S(2*4)E12'2.25'
 DC (1*8)FDL1S30E(-12*2)'2.25'
 DC (1*8)FDL1S30E40'2.25'
 DC 13FL.(13)'2.25')");

	expect_errors(" DC 10FDP(123)L(2*3)S(2*4)E(-12*2)(34)'2.25'");
	expect_errors(" DC 10FDP(123)(1)L(2*3)S(2*4)E(-12*2)'2.25'");
	expect_errors(" DC (1*8)FDL1S(1+2)(3+1)E40'2.25'");
}

TEST(data_definition_grammar, address_nominal)
{
	expect_no_errors(" DC (1*8)S(512(12))");
	expect_no_errors(" DC 8S(512(12))");
	expect_no_errors(" DC S(512(12))");
	expect_no_errors(" DC SP(13)(512(12))");
	expect_no_errors(" DC SP(13)L2(512(12))");
	expect_no_errors(" DC SP(13)L(13)(512(12))");

	expect_errors(" DC SP(13)L(13)(512(12,13))");
}

TEST(data_definition_grammar, expression_nominal)
{
	expect_no_errors("A DC Y(*-A,*+4)");
	expect_no_errors("A DC Y(A+32)");
	expect_no_errors("A DC YL4(A+32)");
	expect_no_errors("A DC YL(4)(A+32)");
	expect_no_errors("A DC 10YL(4)(A+32)");
	expect_no_errors("A DC (1+9)Y(*-A,*+4)");
}

TEST(data_definition_grammar, no_nominal)
{
	expect_no_errors("A DC 0C");
	expect_no_errors("A DC 0CL10");
	expect_no_errors("A DC 0CL(1+10)");
}

TEST(data_definition, duplication_factor)
{
	using namespace hlasm_plugin::parser_library;

	analyzer a("13C'A'");
	auto res = a.parser().data_def();

	auto parsed = std::move(res->value);
	EXPECT_EQ(parsed.diags().size(), (size_t)0);
	
	auto dup_f = parsed.dupl_factor->evaluate(a.context().ord_ctx).get_abs();
	EXPECT_EQ(dup_f, 13);
}

TEST(data_definition, duplication_factor_expr)
{
	using namespace hlasm_plugin::parser_library;

	analyzer a("(13*2)C'A'");
	auto res = a.parser().data_def();

	auto parsed = std::move(res->value);
	EXPECT_EQ(parsed.diags().size(), (size_t)0);

	auto dup_f = parsed.dupl_factor->evaluate(a.context().ord_ctx).get_abs();
	EXPECT_EQ(dup_f, 26);
}

TEST(data_definition, duplication_factor_out_of_range)
{
	using namespace hlasm_plugin::parser_library;

	analyzer a("1231312123123123123C'A'");
	auto res = a.parser().data_def();

	auto parsed = std::move(res->value);
	EXPECT_GT(parsed.diags().size(), (size_t)0);

	auto dup_f = parsed.dupl_factor->evaluate(a.context().ord_ctx).get_abs();
	EXPECT_EQ(dup_f, 1);
}

TEST(data_definition, duplication_factor_invalid_number)
{
	using namespace hlasm_plugin::parser_library;

	analyzer a("-C'A'");
	auto res = a.parser().data_def();

	auto parsed = std::move(res->value);
	EXPECT_GT(parsed.diags().size(), (size_t)0);

	auto dup_f = parsed.dupl_factor->evaluate(a.context().ord_ctx).get_abs();
	EXPECT_EQ(dup_f, 1);
}

TEST(data_definition, all_fields)
{
	using namespace hlasm_plugin::parser_library;

	analyzer a("(1*8)FDP(123)L2S(2*4)E(-12*2)'2.25'");
	auto res = a.parser().data_def();

	auto parsed = std::move(res->value);
	EXPECT_EQ(parsed.diags().size(), (size_t)0);

	auto dup_f = parsed.dupl_factor->evaluate(a.context().ord_ctx).get_abs();
	EXPECT_EQ(dup_f, 8);

	EXPECT_EQ(parsed.program_type->evaluate(a.context().ord_ctx).get_abs(), 123);
	EXPECT_EQ(parsed.length->evaluate(a.context().ord_ctx).get_abs(), 2);
	EXPECT_EQ(parsed.length_type, expressions::data_definition::length_type::BYTE);
	EXPECT_EQ(parsed.scale->evaluate(a.context().ord_ctx).get_abs(), 8);
	EXPECT_EQ(parsed.exponent->evaluate(a.context().ord_ctx).get_abs(), -24);
	ASSERT_NE(parsed.nominal_value->access_string(), nullptr);
	EXPECT_EQ(parsed.nominal_value->access_string()->value, "'2.25'");
}

TEST(data_definition, no_nominal)
{
	using namespace hlasm_plugin::parser_library;

	analyzer a("0FDL2");
	auto res = a.parser().data_def();

	auto parsed = std::move(res->value);
	EXPECT_EQ(parsed.diags().size(), (size_t)0);

	EXPECT_EQ(parsed.dupl_factor->evaluate(a.context().ord_ctx).get_abs(), 0);
	EXPECT_EQ(parsed.program_type, nullptr);
	EXPECT_EQ(parsed.length->evaluate(a.context().ord_ctx).get_abs(), 2);
	EXPECT_EQ(parsed.length_type, expressions::data_definition::length_type::BYTE);
	EXPECT_EQ(parsed.scale, nullptr);
	EXPECT_EQ(parsed.exponent, nullptr);
	ASSERT_EQ(parsed.nominal_value, nullptr);
}

TEST(data_definition, no_nominal_expr)
{
	using namespace hlasm_plugin::parser_library;

	analyzer a("0FDL(2+2)");
	auto res = a.parser().data_def();

	auto parsed = std::move(res->value);
	EXPECT_EQ(parsed.diags().size(), (size_t)0);

	EXPECT_EQ(parsed.dupl_factor->evaluate(a.context().ord_ctx).get_abs(), 0);
	EXPECT_EQ(parsed.program_type, nullptr);
	EXPECT_EQ(parsed.length->evaluate(a.context().ord_ctx).get_abs(), 4);
	EXPECT_EQ(parsed.length_type, expressions::data_definition::length_type::BYTE);
	EXPECT_EQ(parsed.scale, nullptr);
	EXPECT_EQ(parsed.exponent, nullptr);
	ASSERT_EQ(parsed.nominal_value, nullptr);
}

TEST(data_definition, bit_length)
{
	using namespace hlasm_plugin::parser_library;

	analyzer a("(1*8)FDP(123)L.2S-8E(-12*2)'2.25'");
	auto res = a.parser().data_def();

	auto parsed = std::move(res->value);
	EXPECT_EQ(parsed.diags().size(), (size_t)0);

	EXPECT_EQ(parsed.dupl_factor->evaluate(a.context().ord_ctx).get_abs(), 8);

	EXPECT_EQ(parsed.program_type->evaluate(a.context().ord_ctx).get_abs(), 123);
	EXPECT_EQ(parsed.length->evaluate(a.context().ord_ctx).get_abs(), 2);
	EXPECT_EQ(parsed.length_type, expressions::data_definition::length_type::BIT);
	EXPECT_EQ(parsed.scale->evaluate(a.context().ord_ctx).get_abs(), -8);
	EXPECT_EQ(parsed.exponent->evaluate(a.context().ord_ctx).get_abs(), -24);
	ASSERT_NE(parsed.nominal_value->access_string(), nullptr);
	EXPECT_EQ(parsed.nominal_value->access_string()->value, "'2.25'");
}

TEST(data_definition, unexpected_dot)
{
	using namespace hlasm_plugin::parser_library;

	analyzer a("(1*8)FDL.2S.-8E(-12*2)'2.25'");
	auto res = a.parser().data_def();

	auto parsed = std::move(res->value);
	EXPECT_GT(parsed.diags().size(), (size_t)0);

	EXPECT_EQ(parsed.dupl_factor->evaluate(a.context().ord_ctx).get_abs(), 8);

	EXPECT_EQ(parsed.program_type, nullptr);
	EXPECT_EQ(parsed.length->evaluate(a.context().ord_ctx).get_abs(), 2);
	EXPECT_EQ(parsed.length_type, expressions::data_definition::length_type::BIT);
	EXPECT_EQ(parsed.scale->evaluate(a.context().ord_ctx).get_abs(), -8);
	EXPECT_EQ(parsed.exponent->evaluate(a.context().ord_ctx).get_abs(), -24);
	ASSERT_NE(parsed.nominal_value->access_string(), nullptr);
	EXPECT_EQ(parsed.nominal_value->access_string()->value, "'2.25'");
}

TEST(data_definition, unexpected_minus)
{
	using namespace hlasm_plugin::parser_library;

	analyzer a("(1*8)FDL.2S.-E(-12*2)'2.25'");
	auto res = a.parser().data_def();

	auto parsed = std::move(res->value);
	EXPECT_GT(parsed.diags().size(), (size_t)0);
}

TEST(data_definition, wrong_modifier_order)
{
	using namespace hlasm_plugin::parser_library;

	analyzer a("1HL-12P(123)S1'1.25'");
	auto res = a.parser().data_def();

	auto parsed = std::move(res->value);
	EXPECT_GT(parsed.diags().size(), (size_t)0);
}