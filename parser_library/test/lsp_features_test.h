#ifndef HLASMPLUGIN_LSP_FEATURES_TEST_H
#define HLASMPLUGIN_LSP_FEATURES_TEST_H

#include "../src/parse_lib_provider.h"
#include "../src/analyzer.h"
#include "../src/context/instruction.h"

constexpr char* MACRO_FILE = "macro_file.hlasm";
constexpr char* SOURCE_FILE = "source_file.hlasm";

using namespace hlasm_plugin::parser_library;

class mock_parse_lib_provider : public parse_lib_provider
{
public:
	virtual parse_result parse_library(const std::string& library, context::hlasm_context& hlasm_ctx)
	{
		analyzer a(macro_contents, MACRO_FILE, hlasm_ctx);
		a.analyze();
		return true;
	}
private:
	const std::string macro_contents =
R"(   MACRO
       MAC   &VAR
       LR    &VAR,&VAR
       MEND
)";
};

class lsp_features_test : public testing::Test
{
public:
	lsp_features_test() : a(contents, SOURCE_FILE,lib_provider ),
		instruction_count(context::instruction::machine_instructions.size() +
			context::instruction::assembler_instructions.size() +
			context::instruction::ca_instructions.size() +
			context::instruction::mnemonic_codes.size())
	{};

	virtual void SetUp()
	{
		a.analyze();
	}
	virtual void TearDown() {}
protected:
	const std::string contents = 
R"(   MAC  1
&VAR   SETA 5
       LR   &VAR,&VAR REMARK
       AGO  .HERE
       L    1,0(5)
.HERE  ANOP

       MACRO
&LABEL MAC2   &VAR
*THIS IS DOCUMENTATION
       LR     &VAR,&LABEL
       AGO    .HERE
       AGO    .HERE2
.HERE2 ANOP
.HERE  ANOP 
       MEND
 
1      MAC2   2
)";
	std::string content;
	mock_parse_lib_provider lib_provider;
	analyzer a;
	const size_t instruction_count;
};

TEST_F(lsp_features_test, go_to)
{
	// jump from source to macro, MAC instruction
	EXPECT_EQ(semantics::position_uri_s(MACRO_FILE,position(1,7)), a.lsp_processor().go_to_definition(position(0, 4)));
	// no jump
	EXPECT_EQ(semantics::position_uri_s(SOURCE_FILE, position(0, 8)), a.lsp_processor().go_to_definition(position(0, 8)));
	// jump in source, open code, var symbol &VAR
	EXPECT_EQ(semantics::position_uri_s(SOURCE_FILE, position(1, 0)), a.lsp_processor().go_to_definition(position(2, 13)));
	// jump in source, open code, seq symbol .HERE
	EXPECT_EQ(semantics::position_uri_s(SOURCE_FILE, position(5, 0)), a.lsp_processor().go_to_definition(position(3, 13)));
	// jump in source, macro, seq symbol .HERE
	EXPECT_EQ(semantics::position_uri_s(SOURCE_FILE, position(14, 0)), a.lsp_processor().go_to_definition(position(11, 15)));
	// jump in source, macro, var symbol &LABEL
	EXPECT_EQ(semantics::position_uri_s(SOURCE_FILE, position(8, 0)), a.lsp_processor().go_to_definition(position(10, 20)));
	// jump in source, macro, var symbol &VAR
	EXPECT_EQ(semantics::position_uri_s(SOURCE_FILE, position(8, 14)), a.lsp_processor().go_to_definition(position(10, 15)));
}

TEST_F(lsp_features_test, refs)
{
	// the same as go_to test but with references
	// reference MAC, should appear once in source and once in macro file
	EXPECT_EQ(a.lsp_processor().references(position(0, 4)).size(), (size_t)2);
	// no reference, returns the same line where the references command was called
	EXPECT_EQ((size_t)1, a.lsp_processor().references(position(0, 8)).size());
	// source code references for &VAR, appeared three times
	EXPECT_EQ((size_t)3, a.lsp_processor().references(position(2, 13)).size());
	// source code references for .HERE, appeared twice
	EXPECT_EQ((size_t)2, a.lsp_processor().references(position(3, 13)).size());
	// references inside macro def, seq symbol .HERE
	EXPECT_EQ((size_t)2, a.lsp_processor().references(position(11, 15)).size());
	//  references inside macro def, var symbol &LABEL
	EXPECT_EQ((size_t)2, a.lsp_processor().references(position(10, 20)).size());
	//  references inside macro def, var symbol &VAR
	EXPECT_EQ((size_t)2, a.lsp_processor().references(position(10, 15)).size());
}

// 4 cases, instruction, sequence, variable and none
TEST_F(lsp_features_test, hover)
{
	// hover for macro MAC2, contains description and user documentation
	auto result = a.lsp_processor().hover(position(17, 8));
	ASSERT_EQ((size_t)2, result.size());
	// probably not correct as the LABEL should be before MAC2
	// however needs proper fix in completion as well, as it should replace whole line instead of inserting text
	EXPECT_EQ("MAC2 LABEL VAR", result[0]);
	EXPECT_EQ("THIS IS DOCUMENTATION\n", result[1]);

	// hover for sequence symbol, defined even though it is skipped because of the macro parsing (wanted behaviour)
	result = a.lsp_processor().hover(position(12, 15));
	ASSERT_EQ((size_t)2, result.size());
	EXPECT_EQ("HERE2 Defined at line 14", result[0]);
	//empty documentation
	EXPECT_EQ("", result[1]);

	// hover for variable symbol, name and type
	result = a.lsp_processor().hover(position(2, 13));
	ASSERT_EQ((size_t)2, result.size());
	EXPECT_EQ("VAR number", result[0]);
	EXPECT_EQ("", result[1]);

	// no hover on remarks
	result = a.lsp_processor().hover(position(2, 24));
	ASSERT_EQ((size_t)0, result.size());
}

// completion for variable symbols (&), sequence syms (.) and instructions ((S*)(s+)(S*))
TEST_F(lsp_features_test, completion)
{
	// all instructions + 2 newly defined macros
	EXPECT_EQ(instruction_count + 2, a.lsp_processor().completion(position(16, 1), '\0', 1).items.size());
	// current scope detection missing !
	// seq symbols
	EXPECT_EQ((size_t)1, a.lsp_processor().completion(position(6, 0), '.', 2).items.size());
	// var symbols
	EXPECT_EQ((size_t)3, a.lsp_processor().completion(position(10, 0), '&', 2).items.size());
}
#endif