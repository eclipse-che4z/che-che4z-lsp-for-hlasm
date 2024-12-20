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

#include <sstream>
#include <string>

#include "antlr4-runtime.h"
#include "gtest/gtest.h"

#include "../common_testing.h"
#include "analyzer.h"
#include "hlasmparser_multiline.h"
#include "lexing/lexer.h"
#include "lexing/token_stream.h"

using namespace hlasm_plugin::parser_library;

// tests lexer class:
// continuation statements, rewinding, token creation

using parser = parsing::hlasmparser_multiline;

TEST(lexer_test, rntest)
{
    std::string out = R"(ORDSYMBOL
SPACE
ORDSYMBOL
SPACE
SPACE
ORDSYMBOL
SPACE
ORDSYMBOL
EOF
)";

    lexing::lexer l;
    l.reset(lexing::u8string_view_with_newlines("TEST TEST \r\n TEST1 TEST2"), {}, 0);
    lexing::token_stream tokens(&l);
    parser parser(&tokens);

    tokens.fill();

    std::stringstream token_stream;
    for (auto token : tokens.get_tokens())
        token_stream << parser.getVocabulary().getSymbolicName(token->getType()) << std::endl;
    auto token_string = token_stream.str();

    ASSERT_EQ(token_string, out);
}

TEST(lexer_test, unlimited_line)
{
    std::string in =
        R"(LABEL INSTR    2,1,                                                                   THIS SHOULD NOT BE IGNORED
      INSTR    2    REMARK
      INSTR    2,1, TOTO JE REMARK)";
    std::string out = R"(ORDSYMBOL
SPACE
ORDSYMBOL
SPACE
NUM
COMMA
NUM
COMMA
SPACE
ORDSYMBOL
SPACE
ORDSYMBOL
SPACE
ORDSYMBOL
SPACE
ORDSYMBOL
SPACE
ORDSYMBOL
SPACE
ORDSYMBOL
SPACE
NUM
SPACE
ORDSYMBOL
SPACE
ORDSYMBOL
SPACE
NUM
COMMA
NUM
COMMA
SPACE
ORDSYMBOL
SPACE
ORDSYMBOL
SPACE
ORDSYMBOL
EOF
)";

    lexing::lexer l;
    l.reset(lexing::u8string_view_with_newlines(in), {}, 0);
    lexing::token_stream tokens(&l);
    parser parser(&tokens);

    tokens.fill();

    std::stringstream token_stream;
    for (auto token : tokens.get_tokens())
        token_stream << parser.getVocabulary().getSymbolicName(token->getType()) << std::endl;
    auto token_string = token_stream.str();

    ASSERT_EQ(token_string, out);
}

TEST(lexer_test, special_spaces)
{
    lexing::lexer l;
    l.reset(lexing::u8string_view_with_newlines("A\v\f\t LR"), {}, 0);

    while (l.more_tokens())
        ;
    ASSERT_GE(l.token_count(), 3);

    size_t i = 0;

    EXPECT_EQ(l.get_token(i++)->getType(), lexing::lexer::IDENTIFIER);
    EXPECT_EQ(l.get_token(i++)->getType(), lexing::lexer::SPACE);
    EXPECT_EQ(l.get_token(i++)->getType(), lexing::lexer::ORDSYMBOL);
}

TEST(lexer_test, attribute_in_continuation)
{
    std::string in = "       LR                                                           1,L\xff'SYMBOL";

    lexing::lexer l;
    l.reset(lexing::u8string_view_with_newlines(in), {}, 0);

    while (l.more_tokens())
        ;
    ASSERT_EQ(l.token_count(), 10);

    size_t i = 0;

    EXPECT_EQ(l.get_token(i++)->getType(), lexing::lexer::SPACE);
    EXPECT_EQ(l.get_token(i++)->getType(), lexing::lexer::ORDSYMBOL);
    EXPECT_EQ(l.get_token(i++)->getType(), lexing::lexer::SPACE);
    EXPECT_EQ(l.get_token(i++)->getType(), lexing::lexer::NUM);
    EXPECT_EQ(l.get_token(i++)->getType(), lexing::lexer::COMMA);
    EXPECT_EQ(l.get_token(i++)->getType(), lexing::lexer::ORDSYMBOL);
    EXPECT_EQ(l.get_token(i++)->getType(), lexing::lexer::CONTINUATION);
    EXPECT_EQ(l.get_token(i++)->getType(), lexing::lexer::ATTR);
    EXPECT_EQ(l.get_token(i++)->getType(), lexing::lexer::ORDSYMBOL);
}

TEST(lexer_test, bad_continuation)
{
    std::string in =
        R"( SAM31                                                                 X
 err
)";
    analyzer a(in);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E001" }));
    EXPECT_TRUE(matches_message_properties(a.diags(), { range({ 1, 0 }, { 1, 4 }) }, &diagnostic::diag_range));
}
