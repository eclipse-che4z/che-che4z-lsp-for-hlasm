#ifndef HLASMPLUGIN_PARSERLIBARY_COPYTEST_H
#define HLASMPLUGIN_PARSERLIBARY_COPYTEST_H
#include "common_testing.h"

class copy_mock : public parse_lib_provider
{
	const std::string* find_content(const std::string& library) const
	{
		if (library == "COPYR")
			return &content_COPYR;
		else if (library == "COPYF")
			return &content_COPYF;
		else if (library == "COPYD")
			return &content_COPYD;
		else if (library == "COPYREC")
			return &content_COPYREC;
		else if (library == "COPYU")
			return &content_COPYU;
		else if (library == "COPYL")
			return &content_COPYL;
		else if (library == "COPYN")
			return &content_COPYN;
		else if (library == "MAC")
			return &content_MAC;
		else if (library == "COPYM")
			return &content_COPYM;
		else if (library == "COPYJ")
			return &content_COPYJ;
		else if (library == "COPYJF")
			return &content_COPYJF;
		else if (library == "COPYND1")
			return &content_COPYND1;
		else if (library == "COPYND2")
			return &content_COPYND2;
		else
			return nullptr;
	}

public:


	virtual parse_result parse_library(const std::string& library, context::hlasm_context& hlasm_ctx, const library_data data)
	{
		current_content = find_content(library);
		if (!current_content) return false;

		holder.push_back(std::move(a));
		a = std::make_unique<analyzer>(*current_content, library, hlasm_ctx,*this, data);
		a->analyze();
		a->collect_diags();
		return true;
	}
	virtual bool has_library(const std::string& library, context::hlasm_context& hlasm_ctx) const
	{
		return find_content(library);
	}
	std::vector<std::unique_ptr<analyzer>> holder;
	std::unique_ptr<analyzer> a;
private:
	const std::string* current_content;

	const std::string content_COPYR =
		R"(   
 LR 1,1
 MACRO
 M1
 LR 1,1
 
 MACRO
 M2
 LR 2,2
 MEND
 AGO .A
.A ANOP
 MEND

&VARX SETA &VARX+1
.A ANOP
.B ANOP
&VAR SETA &VAR+1
)";
	const std::string content_COPYF =
		R"(  
 LR 1,1
&VARX SETA &VARX+1
 COPY COPYR
&VAR SETA &VAR+1
.C ANOP
)";

	const std::string content_COPYD =
		R"(  

 LR 1,
)";

	const std::string content_COPYREC =
		R"(  
 ANOP
 COPY COPYREC
 ANOP
)";

	const std::string content_COPYU =
		R"(  
 ANOP
 MACRO
 M
 MEND
 MEND
 ANOP
)";

	const std::string content_COPYL =
		R"(  
 LR 1,1
.A ANOP
&VARX SETA &VARX+1
 AGO .X
&VAR SETA &VAR+1
.A ANOP
.C ANOP
)";

	const std::string content_COPYN =
		R"( 
 MAC
)";

	const std::string content_MAC =
		R"( MACRO
 MAC
 LR 1,1
 COPY COPYM
 MEND
)";

	const std::string content_COPYM =
		R"(
.A ANOP
 GBLA &X
&X SETA 4
)";

	const std::string content_COPYJ =
		R"(
 AGO .X
 LR
.X ANOP
)";
	const std::string content_COPYJF =
		R"(
 AGO .X
 LR
)";

	const std::string content_COPYND1 =
		R"(
 COPY COPYND2
)";

	const std::string content_COPYND2 =
		R"(



 LR 1,)";
};

TEST(copy, copy_enter_fail)
{
	std::string input =
		R"(
 COPY A+1
 COPY UNKNOWN
)";
	copy_mock mock;
	analyzer a(input,"",mock);
	a.analyze();

	a.collect_diags();

	EXPECT_EQ(a.context().copy_members().size(), (size_t)0);
	EXPECT_EQ(a.context().whole_copy_stack().size(), (size_t)0);

	EXPECT_EQ(a.diags().size(), (size_t)2);
}

TEST(copy, copy_enter_success)
{
	std::string input =
		R"(
 COPY COPYR
)";
	copy_mock mock;
	analyzer a(input, "", mock);
	a.analyze();

	a.collect_diags();

	EXPECT_EQ(a.context().copy_members().size(), (size_t)1);

	EXPECT_EQ(a.context().macros().size(), (size_t)1);

	EXPECT_TRUE(a.context().get_sequence_symbol(a.context().ids().add("A")));

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(copy, copy_enter_diag_test)
{
	std::string input =
		R"(
 COPY COPYD
)";
	copy_mock mock;
	analyzer a(input, "start", mock);
	a.analyze();

	a.collect_diags();

	EXPECT_EQ(a.context().copy_members().size(), (size_t)1);

	ASSERT_EQ(a.diags().size(), (size_t)1);

	auto diag = a.diags()[0];

	EXPECT_EQ(a.diags()[0].diag_range.start.line, (position_t)2);
	EXPECT_EQ(a.diags()[0].file_name,"COPYD");
	EXPECT_EQ(a.diags()[0].related.size(), (size_t)1);
	EXPECT_EQ(a.diags()[0].related[0].location.rang.start.line, (position_t)1);
	EXPECT_EQ(a.diags()[0].related[0].location.uri,"start");
}

TEST(copy, copy_jump)
{
	std::string input =
		R"(
&VARX SETA 0
&VAR SETA 0
 COPY COPYF
 AIF (&VAR LT 4).A
)";
	copy_mock mock;
	analyzer a(input, "", mock);
	a.analyze();

	a.collect_diags();

	EXPECT_EQ(a.context().copy_members().size(), (size_t)2);

	EXPECT_EQ(a.diags().size(), (size_t)0);

	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("VAR"))->access_set_symbol_base()->access_set_symbol<context::A_t>()->get_value(), 4);
	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("VARX"))->access_set_symbol_base()->access_set_symbol<context::A_t>()->get_value(), 2);
}

TEST(copy, copy_unbalanced_macro)
{
	std::string input =
		R"(
 COPY COPYU
)";
	copy_mock mock;
	analyzer a(input, "", mock);
	a.analyze();

	a.collect_diags();

	EXPECT_EQ(a.context().copy_members().size(), (size_t)1);

	EXPECT_EQ(a.diags().size(), (size_t)0);

	EXPECT_EQ(mock.a->diags().size(), (size_t)1);
}

TEST(copy, copy_twice)
{
	std::string input =
		R"(
 COPY COPYR
 COPY COPYR
)";
	copy_mock mock;
	analyzer a(input, "", mock);
	a.analyze();

	a.collect_diags();

	EXPECT_EQ(a.context().copy_members().size(), (size_t)1);

	EXPECT_EQ(a.diags().size(), (size_t)2);
}

TEST(copy, macro_call_from_copy_enter)
{
	std::string input =
		R"(
 COPY COPYR
 M1
 M2
)";
	copy_mock mock;
	analyzer a(input, "", mock);
	a.analyze();

	a.collect_diags();

	EXPECT_EQ(a.context().copy_members().size(), (size_t)1);

	EXPECT_EQ(a.diags().size(), (size_t)0);

	EXPECT_EQ(a.context().macros().size(), (size_t)2);
}

TEST(copy, copy_enter_from_macro_call)
{
	std::string input =
		R"(
 MACRO
 M
 LR 1,1
.B ANOP
 COPY COPYR
 MEND

 M
)";
	copy_mock mock;
	analyzer a(input, "start", mock);
	a.analyze();

	a.collect_diags();

	EXPECT_EQ(a.context().copy_members().size(), (size_t)1);

	EXPECT_EQ(a.context().macros().size(), (size_t)2);

	auto mac = a.context().macros().find(a.context().ids().add("M"));
	ASSERT_TRUE(mac!= a.context().macros().end());

	EXPECT_TRUE(mac->second->labels.find(a.context().ids().add("A")) != mac->second->labels.end());
	EXPECT_TRUE(mac->second->labels.find(a.context().ids().add("B")) != mac->second->labels.end());

	ASSERT_EQ(a.diags().size(), (size_t)1);

	EXPECT_EQ(a.diags()[0].diag_range.start.line, (position_t)16);
	EXPECT_EQ(a.diags()[0].file_name, "COPYR");
	ASSERT_EQ(a.diags()[0].related.size(), (size_t)1);
	EXPECT_EQ(a.diags()[0].related[0].location.rang.start.line, (position_t)5);
	EXPECT_EQ(a.diags()[0].related[0].location.uri, "start");
}

TEST(copy, copy_enter_from_lookahead)
{
	std::string input =
		R"(
&V SETA 0
 AGO .C
&V SETA &V+1
 COPY COPYL
&V SETA &V+1
 
)";
	copy_mock mock;
	analyzer a(input, "start", mock);
	a.analyze();

	a.collect_diags();

	EXPECT_EQ(a.context().copy_members().size(), (size_t)1);

	EXPECT_EQ(a.context().get_var_sym(a.context().ids().add("V"))->access_set_symbol_base()->access_set_symbol<context::A_t>()->get_value(), 1);

	ASSERT_EQ(a.diags().size(), (size_t)1);

	EXPECT_EQ(a.diags()[0].diag_range.start.line, (position_t)6);
	EXPECT_EQ(a.diags()[0].file_name, "COPYL");
	ASSERT_EQ(a.diags()[0].related.size(), (size_t)1);
	EXPECT_EQ(a.diags()[0].related[0].location.rang.start.line, (position_t)4);
	EXPECT_EQ(a.diags()[0].related[0].location.uri, "start");
}

TEST(copy, nested_macro_copy_call)
{
	std::string input =
		R"(
 COPY COPYN
 
)";
	copy_mock mock;
	analyzer a(input, "", mock);
	a.analyze();

	a.collect_diags();

	EXPECT_EQ(a.context().copy_members().size(), (size_t)2);
	ASSERT_EQ(a.context().macros().size(), (size_t)1);
	auto mac = a.context().macros().find(a.context().ids().add("MAC"));

	EXPECT_TRUE(mac->second->labels.find(a.context().ids().add("A")) != mac->second->labels.end());

	EXPECT_EQ(a.context().globals().find(a.context().ids().add("X"))->second->access_set_symbol_base()->access_set_symbol<context::A_t>()->get_value(), 4);

	EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(copy, inner_copy_jump)
{
	std::string input =
		R"(
 COPY COPYJ
 LR
 
)";
	copy_mock mock;
	analyzer a(input, "", mock);
	a.analyze();

	a.collect_diags();

	EXPECT_EQ(a.context().copy_members().size(), (size_t)1);

	EXPECT_EQ(a.diags().size(), (size_t)1);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
}

TEST(copy, jump_from_copy_fail)
{
	std::string input =
		R"(
 COPY COPYJF
)";
	copy_mock mock;
	analyzer a(input, "start", mock);
	a.analyze();

	a.collect_diags();

	EXPECT_EQ(a.context().copy_members().size(), (size_t)1);

	EXPECT_EQ(a.diags().size(), (size_t)2);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

	EXPECT_EQ(a.diags()[1].diag_range.start.line, (position_t)2);
	EXPECT_EQ(a.diags()[1].file_name, "COPYJF");
	ASSERT_EQ(a.diags()[1].related.size(), (size_t)1);
	EXPECT_EQ(a.diags()[1].related[0].location.rang.start.line, (position_t)1);
	EXPECT_EQ(a.diags()[1].related[0].location.uri, "start");

	EXPECT_EQ(a.diags()[0].diag_range.start.line, (position_t)1);
	EXPECT_EQ(a.diags()[0].file_name, "COPYJF");
	ASSERT_EQ(a.diags()[0].related.size(), (size_t)1);
	EXPECT_EQ(a.diags()[0].related[0].location.rang.start.line, (position_t)1);
	EXPECT_EQ(a.diags()[0].related[0].location.uri, "start");
}

TEST(copy, jump_in_macro_from_copy_fail)
{
	std::string input =
		R"(
 MACRO
 m
 copy copyJF
 mend

 m
)";
	copy_mock mock;
	analyzer a(input, "start", mock);
	a.analyze();

	a.collect_diags();

	EXPECT_EQ(a.context().copy_members().size(), (size_t)1);

	EXPECT_EQ(a.diags().size(), (size_t)2);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

	EXPECT_EQ(a.diags()[0].diag_range.start.line, (position_t)1);
	EXPECT_EQ(a.diags()[0].file_name, "COPYJF");
	ASSERT_EQ(a.diags()[0].related.size(), (size_t)2);
	EXPECT_EQ(a.diags()[0].related[0].location.rang.start.line, (position_t)3);
	EXPECT_EQ(a.diags()[0].related[0].location.uri, "start");
	EXPECT_EQ(a.diags()[0].related[1].location.rang.start.line, (position_t)6);
	EXPECT_EQ(a.diags()[0].related[1].location.uri, "start");
}

TEST(copy, macro_nested_diagnostics)
{
	std::string input =
		R"( MACRO
 MAC

 copy COPYND1

 MEND
 
 MAC  
)";
	copy_mock mock;
	analyzer a(input, "start", mock);
	a.analyze();

	a.collect_diags();

	EXPECT_EQ(a.context().copy_members().size(), (size_t)2);

	EXPECT_EQ(a.diags().size(), (size_t)1);
	EXPECT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);

	EXPECT_EQ(a.diags()[0].diag_range.start.line, (position_t)4);
	EXPECT_EQ(a.diags()[0].file_name, "COPYND2");
	ASSERT_EQ(a.diags()[0].related.size(), (size_t)3);
	EXPECT_EQ(a.diags()[0].related[0].location.rang.start.line, (position_t)1);
	EXPECT_EQ(a.diags()[0].related[0].location.uri, "COPYND1");
	EXPECT_EQ(a.diags()[0].related[1].location.rang.start.line, (position_t)3);
	EXPECT_EQ(a.diags()[0].related[1].location.uri, "start");
	EXPECT_EQ(a.diags()[0].related[2].location.rang.start.line, (position_t)7);
	EXPECT_EQ(a.diags()[0].related[2].location.uri, "start");
}

#endif
