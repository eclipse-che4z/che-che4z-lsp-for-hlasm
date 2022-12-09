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
#include <vector>

#include "gtest/gtest.h"

#include "lsp/completion_item.h"
#include "lsp/file_info.h"
#include "lsp/item_convertors.h"
#include "lsp/macro_info.h"
#include "lsp/text_data_view.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library::lsp;


TEST(item_convertors, macro_sequence_symbol)
{
    const macro_sequence_symbol seq(id_index("INMAC"), location(), 0);
    const lsp::completion_item_s expected(".INMAC", "Sequence symbol", ".INMAC", "", completion_item_kind::seq_sym);

    EXPECT_EQ(generate_completion_item(seq), expected);
}

TEST(item_convertors, opencode_sequence_symbol)
{
    const opencode_sequence_symbol seq(id_index("OUTMAC"), location(), source_position(), source_snapshot());
    const lsp::completion_item_s expected(".OUTMAC", "Sequence symbol", ".OUTMAC", "", completion_item_kind::seq_sym);

    EXPECT_EQ(generate_completion_item(seq), expected);
}

TEST(item_convertors, macro_param)
{
    const variable_symbol_definition var(id_index("LABEL"), 0, position());
    const lsp::completion_item_s expected("&LABEL", "MACRO parameter", "&LABEL", "", completion_item_kind::var_sym);


    EXPECT_EQ(generate_completion_item(var), expected);
}

TEST(item_convertors, set_symbol_a)
{
    const variable_symbol_definition var(id_index("KEY_PAR"), SET_t_enum::A_TYPE, false, 0, position());
    const lsp::completion_item_s expected("&KEY_PAR", "SETA variable", "&KEY_PAR", "", completion_item_kind::var_sym);


    EXPECT_EQ(generate_completion_item(var), expected);
}

TEST(item_convertors, set_symbol_b)
{
    const variable_symbol_definition var(id_index("KEY_PAR"), SET_t_enum::B_TYPE, false, 0, position());
    const lsp::completion_item_s expected("&KEY_PAR", "SETB variable", "&KEY_PAR", "", completion_item_kind::var_sym);


    EXPECT_EQ(generate_completion_item(var), expected);
}

TEST(item_convertors, set_symbol_c)
{
    const variable_symbol_definition var(id_index("KEY_PAR"), SET_t_enum::C_TYPE, false, 0, position());
    const lsp::completion_item_s expected("&KEY_PAR", "SETC variable", "&KEY_PAR", "", completion_item_kind::var_sym);


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

const std::string macro_def_expected = R"(```
       MAC     &FIRST_PARAM,      first param remark                   X
               &SECOND_PARAM=1    second param remark
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
        label_storage {},
        location(position(4, 0), hlasm_plugin::utils::resource::resource_location()),
        std::unordered_set<std::shared_ptr<copy_member>> {});
    macro_info mi(
        false, location(position(4, 0), hlasm_plugin::utils::resource::resource_location()), mac_def, {}, {}, {});
    file_info fi(mac_def, text_data_view(macro_def_input));

    auto result_with_doc = generate_completion_item(mi, &fi);
    auto result_without_doc = generate_completion_item(mi, nullptr);

    lsp::completion_item_s expected_with_doc(
        "MAC", "MAC &FIRST_PARAM,&SECOND_PARAM=1", "MAC", macro_def_expected, completion_item_kind::macro);
    lsp::completion_item_s expected_without_doc(
        "MAC", "MAC &FIRST_PARAM,&SECOND_PARAM=1", "MAC", "", completion_item_kind::macro);

    EXPECT_EQ(result_with_doc, expected_with_doc);
    EXPECT_EQ(result_without_doc, expected_without_doc);
}
