/*
 * Copyright (c) 2022 Broadcom.
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

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "completion_item.h"
#include "context/using.h"
#include "lsp/file_info.h"
#include "lsp/item_convertors.h"
#include "lsp/macro_info.h"
#include "lsp/text_data_view.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library::lsp;

constexpr auto zero_stmt_id = context::statement_id { 0 };
TEST(item_convertors, macro_sequence_symbol)
{
    const macro_sequence_symbol seq { location(), zero_stmt_id };
    const completion_item expected(".INMAC", "Sequence symbol", ".INMAC", "", completion_item_kind::seq_sym);

    EXPECT_EQ(generate_completion_item(id_index("INMAC"), seq), expected);
}

TEST(item_convertors, opencode_sequence_symbol)
{
    const opencode_sequence_symbol seq { location(), source_position(), source_snapshot() };
    const completion_item expected(".OUTMAC", "Sequence symbol", ".OUTMAC", "", completion_item_kind::seq_sym);

    EXPECT_EQ(generate_completion_item(id_index("OUTMAC"), seq), expected);
}

TEST(item_convertors, macro_param)
{
    const variable_symbol_definition var(id_index("LABEL"), zero_stmt_id, position());
    const completion_item expected("&LABEL", "MACRO parameter", "&LABEL", "", completion_item_kind::var_sym);


    EXPECT_EQ(generate_completion_item(var), expected);
}

TEST(item_convertors, set_symbol_a)
{
    const variable_symbol_definition var(id_index("KEY_PAR"), SET_t_enum::A_TYPE, false, zero_stmt_id, position());
    const completion_item expected("&KEY_PAR", "SETA variable", "&KEY_PAR", "", completion_item_kind::var_sym);


    EXPECT_EQ(generate_completion_item(var), expected);
}

TEST(item_convertors, set_symbol_b)
{
    const variable_symbol_definition var(id_index("KEY_PAR"), SET_t_enum::B_TYPE, false, zero_stmt_id, position());
    const completion_item expected("&KEY_PAR", "SETB variable", "&KEY_PAR", "", completion_item_kind::var_sym);


    EXPECT_EQ(generate_completion_item(var), expected);
}

TEST(item_convertors, set_symbol_c)
{
    const variable_symbol_definition var(id_index("KEY_PAR"), SET_t_enum::C_TYPE, false, zero_stmt_id, position());
    const completion_item expected("&KEY_PAR", "SETC variable", "&KEY_PAR", "", completion_item_kind::var_sym);


    EXPECT_EQ(generate_completion_item(var), expected);
}

namespace {

const std::string macro_def_input =
    R"(
* Before macro line 1
* Before macro line 2
       MACRO
       MAC     &FIRST_PARAM,      first param remark                   X
               &SECOND_PARAM=1    second param remark
* After macro line 1
.* After macro line 2
       MEND

       MAC
 
)";

const std::string macro_def_expected = R"(```hlasm
       MAC     &FIRST_PARAM,      first param remark                   X
               &SECOND_PARAM=1    second param remark
```
```hlasm
* Before macro line 1
* Before macro line 2
* After macro line 1
.* After macro line 2
```
)";
} // namespace

TEST(item_convertors, macro_doc)
{
    text_data_view text(macro_def_input);
    EXPECT_EQ(get_macro_documentation(text, 4), macro_def_expected);
}

TEST(item_convertors, macro_signature_with_label)
{
    std::vector<macro_arg> params;
    params.emplace_back(macro_arg(nullptr, id_index("FIRST_PARAM")));
    params.emplace_back(macro_arg(std::make_unique<macro_param_data_single>("1"), id_index("SECOND_PARAM")));
    macro_definition def(id_index("MAC"), id_index("LABEL"), std::move(params), {}, {}, {}, {}, {});

    EXPECT_EQ(get_macro_signature(def), "&LABEL MAC &FIRST_PARAM,&SECOND_PARAM=1");
}

TEST(item_convertors, macro_signature_without_label)
{
    std::vector<macro_arg> params;
    params.emplace_back(macro_arg(nullptr, id_index("FIRST_PARAM")));
    params.emplace_back(macro_arg(std::make_unique<macro_param_data_single>("1"), id_index("SECOND_PARAM")));
    macro_definition def(id_index("MAC"), id_index(), std::move(params), {}, {}, {}, {}, {});

    EXPECT_EQ(get_macro_signature(def), "MAC &FIRST_PARAM,&SECOND_PARAM=1");
}

TEST(item_convertors, macro)
{
    std::vector<macro_arg> params;
    params.emplace_back(macro_arg(nullptr, id_index("FIRST_PARAM")));
    params.emplace_back(macro_arg(std::make_unique<macro_param_data_single>("1"), id_index("SECOND_PARAM")));
    auto mac_def = std::make_shared<macro_definition>(id_index("MAC"),
        id_index(),
        std::move(params),
        statement_block {},
        copy_nest_storage {},
        macro_label_storage {},
        location(position(4, 0), hlasm_plugin::utils::resource::resource_location()),
        std::unordered_set<std::shared_ptr<copy_member>> {});
    macro_info mi(
        false, location(position(4, 0), hlasm_plugin::utils::resource::resource_location()), mac_def, {}, {}, {});
    file_info fi(mac_def, text_data_view(macro_def_input));

    auto result_with_doc = generate_completion_item(mi, &fi);
    auto result_without_doc = generate_completion_item(mi, nullptr);

    completion_item expected_with_doc(
        "MAC", "MAC &FIRST_PARAM,&SECOND_PARAM=1", "MAC", macro_def_expected, completion_item_kind::macro);
    completion_item expected_without_doc(
        "MAC", "MAC &FIRST_PARAM,&SECOND_PARAM=1", "MAC", "", completion_item_kind::macro);

    EXPECT_EQ(result_with_doc, expected_with_doc);
    EXPECT_EQ(result_without_doc, expected_without_doc);
}

TEST(item_convertors, using_hover_text)
{
    using namespace std::string_view_literals;
    constexpr id_index A("A"), B("B"), D("D"), E("E"), L("L"), Z("Z");
    const std::array input = {
        std::pair(using_context_description { {}, A, 5, 3, 0, { 1, 2 } }, "**A**+X'5'(X'3'),R1,R2"sv),
        std::pair(using_context_description { {}, E, 0, 2, 1, { 1, 2 } }, "**E**(X'2'),R1+X'1',R2"sv),
        std::pair(using_context_description { L, D, 0, 4096, 0, { 3 } }, "**L.D**,R3"sv),
        std::pair(using_context_description { Z, {}, 15, 4096, 0, { 7 } }, "**Z.**+X'F',R7"sv),
        std::pair(using_context_description { {}, B, -5, 4096, 0, { 12 } }, "**B**-X'5',R12"sv),
    };

    for (const auto& [in, expected] : input)
    {
        std::string out;
        lsp::append_hover_text(out, in);
        EXPECT_EQ(out, expected);
    }

    EXPECT_EQ(lsp::hover_text(std::span<const context::using_context_description>()), "");
    EXPECT_EQ(lsp::hover_text(std::span(&input.front().first, 1)), "Active USINGs: **A**+X'5'(X'3'),R1,R2\n");
}
