/*
 * Copyright (c) 2021 Broadcom.
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

#include <algorithm>
#include <iterator>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "../common_testing.h"
#include "../workspace/empty_configs.h"
#include "analyzer.h"
#include "context/hlasm_context.h"
#include "context/id_storage.h"
#include "context/well_known.h"
#include "library_mock.h"
#include "lsp/lsp_context.h"
#include "lsp/opencode_info.h"
#include "lsp/text_data_view.h"
#include "utils/resource_location.h"
#include "workspaces/file.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/workspace.h"
#include "workspaces/workspace_configuration.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::utils::hashers;
using namespace hlasm_plugin::utils::resource;

namespace {
std::vector<diagnostic> extract_diags(workspace& ws, workspace_configuration& cfg)
{
    std::vector<diagnostic> result;
    cfg.produce_diagnostics(result, {}, {});
    ws.produce_diagnostics(result);
    return result;
}

analyzing_context create_analyzing_context(std::string file_name, std::shared_ptr<context::id_storage> ids)
{
    auto hlasm_ctx =
        std::make_shared<context::hlasm_context>(resource_location(file_name), asm_option(), std::move(ids));
    return analyzing_context {
        hlasm_ctx,
        std::make_shared<lsp::lsp_context>(hlasm_ctx),
    };
}

auto parse_dependency(std::shared_ptr<file> f, analyzing_context ctx, processing::processing_kind proc_kind)
{
    auto filename = f->get_location().filename();
    auto filename_id = ctx.hlasm_ctx->add_id(filename);
    std::pair result {
        std::make_unique<analyzer>(f->get_text(),
            analyzer_options {
                f->get_location(),
                std::move(ctx),
                analyzer_options::dependency(std::move(filename), proc_kind),
            }),
        std::pair { proc_kind, std::move(filename_id) },
    };

    result.first->analyze();

    return result;
}

void save_dependency(macro_cache& cache,
    std::pair<std::unique_ptr<analyzer>, std::pair<processing::processing_kind, context::id_index>> input)
{
    auto key = macro_cache_key::create_from_context(
        *input.first->context().hlasm_ctx, input.second.first, input.second.second);

    cache.save_macro(key, *input.first);
}

auto open_file(resource_location file, std::string text, file_manager& fm)
{
    fm.did_open_file(file, 0, text);
    return fm.find(file);
}

} // namespace

TEST(macro_cache_test, copy_from_macro)
{
    std::string opencode_file_name = "opencode";
    resource_location opencode_file_loc(opencode_file_name);

    std::string macro_file_name = "lib/MAC";
    resource_location macro_file_loc(macro_file_name);
    std::string macro_text =
        R"( MACRO
       MAC &PARAM
       COPY COPYFILE
       MEND
)";
    std::string copyfile_file_name = "lib/COPYFILE";
    resource_location copyfile_file_loc(copyfile_file_name);
    std::string copyfile_text =
        R"(
       LR 15,1
)";

    file_manager_impl file_mngr;

    auto macro_file = open_file(macro_file_loc, macro_text, file_mngr);
    auto copy_file = open_file(copyfile_file_loc, copyfile_text, file_mngr);

    macro_cache macro_c(file_mngr, macro_file);
    macro_cache copy_c(file_mngr, copy_file);

    auto ids = std::make_shared<context::id_storage>();

    // parse everything and cache results
    analyzing_context ctx = create_analyzing_context(opencode_file_name, ids);
    save_dependency(copy_c, parse_dependency(copy_file, ctx, processing::processing_kind::COPY));
    save_dependency(macro_c, parse_dependency(macro_file, ctx, processing::processing_kind::MACRO));

    constexpr context::id_index macro_id("MAC");
    constexpr context::id_index copy_id("COPYFILE");
    macro_cache_key macro_key { processing::processing_kind::MACRO, macro_id, {} };
    macro_cache_key copy_key { processing::processing_kind::COPY, copy_id, {} };

    // try recalling the cached results
    analyzing_context new_ctx = create_analyzing_context(opencode_file_name, ids);
    EXPECT_EQ(macro_c.load_from_cache(macro_key, new_ctx), std::vector { copy_file });
    EXPECT_NE(new_ctx.hlasm_ctx->get_macro_definition(macro_id), nullptr);
    EXPECT_NE(new_ctx.lsp_ctx->get_macro_info(macro_id), nullptr);
    EXPECT_NE(new_ctx.hlasm_ctx->get_copy_member(copy_id), nullptr);

    // introduce macro change
    document_change simple_change({}, " ");
    file_mngr.did_change_file(macro_file_loc, 0, std::span(&simple_change, 1));

    // After macro change, copy should still be cached
    analyzing_context ctx_macro_changed = create_analyzing_context(opencode_file_name, ids);
    EXPECT_TRUE(copy_c.load_from_cache(copy_key, ctx_macro_changed));
    EXPECT_NE(ctx_macro_changed.hlasm_ctx->get_copy_member(copy_id), nullptr);
    EXPECT_FALSE(macro_c.load_from_cache(macro_key, ctx_macro_changed));


    // Reparse the change so everything is cached again
    save_dependency(macro_c, parse_dependency(macro_file, ctx, processing::processing_kind::MACRO));

    // introduce change into copy
    file_mngr.did_change_file(copyfile_file_loc, 0, std::span(&simple_change, 1));

    // Macro depends on the copyfile, so none should be cached.
    analyzing_context ctx_copy_changed = create_analyzing_context(opencode_file_name, ids);
    EXPECT_FALSE(macro_c.load_from_cache(macro_key, ctx_copy_changed));
    EXPECT_FALSE(copy_c.load_from_cache(copy_key, ctx_copy_changed));
}

TEST(macro_cache_test, opsyn_change)
{
    std::string opencode_file_name = "opencode";
    resource_location opencode_file_loc(opencode_file_name);
    ///* Simulates the following code
    /// SETA   OPSYN LR
    ///       MAC 1
    std::string macro_file_name = "lib/MAC";
    resource_location macro_file_loc(macro_file_name);
    std::string macro_text =
        R"( MACRO
       MAC &PARAM
       MEND
)";
    file_manager_impl file_mngr;
    auto macro_file = open_file(macro_file_loc, macro_text, file_mngr);
    macro_cache macro_c(file_mngr, macro_file);
    constexpr context::id_index macro_id("MAC");

    auto ids = std::make_shared<context::id_storage>();
    constexpr context::id_index L("L");
    constexpr context::id_index LR("LR");
    constexpr context::id_index SETA("SETA");
    constexpr context::id_index SETB("SETB");

    // generate OPSYN condition and parse macro
    {
        analyzing_context ctx = create_analyzing_context(opencode_file_name, ids);
        EXPECT_TRUE(ctx.hlasm_ctx->add_mnemonic(SETA, LR));
        save_dependency(macro_c, parse_dependency(macro_file, ctx, processing::processing_kind::MACRO));
    }

    // try loading with and without OPSYN change
    macro_cache_key macro_key { processing::processing_kind::MACRO, macro_id, {} };
    macro_cache_key macro_key_one_opsyn {
        processing::processing_kind::MACRO,
        macro_id,
        { cached_opsyn_mnemo { context::well_known::SETA, LR, false } },
    };
    {
        analyzing_context ctx = create_analyzing_context(opencode_file_name, ids);
        EXPECT_FALSE(macro_c.load_from_cache(macro_key, ctx));
        EXPECT_TRUE(macro_c.load_from_cache(macro_key_one_opsyn, ctx));
    }

    // parse macro with extra opsyn
    {
        analyzing_context ctx = create_analyzing_context(opencode_file_name, ids);
        EXPECT_TRUE(ctx.hlasm_ctx->add_mnemonic(L, SETB));
        EXPECT_TRUE(ctx.hlasm_ctx->add_mnemonic(SETA, LR));
        save_dependency(macro_c, parse_dependency(macro_file, ctx, processing::processing_kind::MACRO));
    }

    // original cache entry is still present
    {
        analyzing_context ctx = create_analyzing_context(opencode_file_name, ids);
        EXPECT_TRUE(macro_c.load_from_cache(macro_key_one_opsyn, ctx));
    }

    // new cache entry is present as well
    {
        macro_cache_key macro_key_two_opsyns = macro_key_one_opsyn;
        macro_key_two_opsyns.opsyn_state.push_back({ L, context::well_known::SETB, false });
        macro_cache_key::sort_opsyn_state(macro_key_two_opsyns.opsyn_state);

        analyzing_context ctx = create_analyzing_context(opencode_file_name, ids);
        EXPECT_TRUE(macro_c.load_from_cache(macro_key_two_opsyns, ctx));
    }
}

TEST(macro_cache_test, empty_macro)
{
    std::string opencode_file_name = "opencode";
    resource_location opencode_file_loc("opencode");
    std::string opencode_text = R"(
 MAC
 MAC)";

    // This tests a caveat where parse_library is called twice for the same macro, if the macro is not defined in its
    // file.
    std::string macro_file_name = "lib/MAC";
    resource_location macro_file_loc(macro_file_name);
    std::string macro_text = "";

    file_manager_impl file_mngr;
    auto macro_file = open_file(macro_file_loc, macro_text, file_mngr);
    macro_cache macro_c(file_mngr, macro_file);
    constexpr context::id_index macro_id("MAC");

    auto ids = std::make_shared<context::id_storage>();


    analyzing_context ctx = create_analyzing_context(opencode_file_name, ids);
    save_dependency(macro_c, parse_dependency(macro_file, ctx, processing::processing_kind::MACRO));

    analyzing_context new_ctx = create_analyzing_context(opencode_file_name, ids);

    macro_cache_key macro_key { processing::processing_kind::MACRO, macro_id, {} };
    EXPECT_TRUE(macro_c.load_from_cache(macro_key, new_ctx));
    EXPECT_FALSE(new_ctx.hlasm_ctx->find_macro(macro_id));
}

TEST(macro_cache_test, get_opsyn_state)
{
    std::string opencode_text =
        R"(
SETA   OPSYN LR
L      OPSYN SETB
 MACRO
 SETC
 MEND
 
 MACRO
 MAC
 MEND
MAC OPSYN AREAD
)";

    analyzer a(opencode_text);
    a.analyze();
    auto state = macro_cache_key::get_opsyn_state(a.hlasm_ctx());

    constexpr context::id_index L("L");
    constexpr context::id_index LR("LR");
    constexpr context::id_index MAC("MAC");
    std::vector<cached_opsyn_mnemo> expected {
        { context::well_known::SETA, LR, false },
        { L, context::well_known::SETB, false },
        { context::well_known::SETC, context::well_known::SETC, true },
        { MAC, context::well_known::AREAD, false },
    };

    macro_cache_key::sort_opsyn_state(expected);

    EXPECT_EQ(state, expected);
}

namespace {
std::optional<diagnostic> find_diag_with_filename(const std::vector<diagnostic>& diags, const resource_location& file)
{
    auto macro_diag = std::ranges::find(diags, file.get_uri(), &diagnostic::file_uri);
    if (macro_diag == diags.end())
        return std::nullopt;
    else
        return *macro_diag;
}

} // namespace

TEST(macro_cache_test, overwrite_by_inline)
{
    auto opencode_file_loc = resource_location("opencode");
    std::string opencode_text =
        R"(
       MAC

       MACRO
       MAC
       LR 1,16
       MEND
       
       MAC
)";
    auto macro_file_loc = resource_location("MAC");
    std::string macro_text =
        R"( MACRO
       MAC
       LR 1,16
       MEND
)";

    file_manager_impl file_mngr;

    file_mngr.did_open_file(opencode_file_loc, 0, opencode_text);
    file_mngr.did_open_file(macro_file_loc, 0, macro_text);

    shared_json global_settings = make_empty_shared_json();
    lib_config config;
    resource_location lib_loc("");
    using namespace ::testing;
    auto library = std::make_shared<NiceMock<library_mock>>();

    EXPECT_CALL(*library, get_location).WillOnce(ReturnRef(lib_loc));

    workspace_configuration ws_cfg(file_mngr, global_settings, config, library);
    workspace ws(file_mngr, ws_cfg);

    EXPECT_CALL(*library, has_file(std::string_view("MAC"), _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(macro_file_loc), Return(true)));

    run_if_valid(ws.did_open_file(opencode_file_loc));
    parse_all_files(ws);

    auto diags = extract_diags(ws, ws_cfg);
    EXPECT_EQ(diags.size(), 2U);
    EXPECT_TRUE(find_diag_with_filename(diags, macro_file_loc));
    EXPECT_TRUE(find_diag_with_filename(diags, opencode_file_loc));

    run_if_valid(ws.mark_file_for_parsing(opencode_file_loc, file_content_state::changed_content));
    parse_all_files(ws);

    diags = extract_diags(ws, ws_cfg);
    EXPECT_EQ(diags.size(), 2U);
    EXPECT_TRUE(find_diag_with_filename(diags, macro_file_loc));
    EXPECT_TRUE(find_diag_with_filename(diags, opencode_file_loc));
}

TEST(macro_cache_test, inline_depends_on_copy)
{
    std::string opencode_file_name = "opencode";
    resource_location opencode_file_loc(opencode_file_name);
    std::string opencode_text =
        R"(
       MACRO 
       MAC
       COPY COPYFILE
       MEND

       MAC
)";
    std::string copy_file_name = "lib/COPYFILE";
    resource_location copy_file_loc(copy_file_name);
    std::string copy_text = R"( LR 1,1 arbitrary instruction)";

    file_manager_impl file_mngr;

    auto copy_file = open_file(copy_file_loc, copy_text, file_mngr);
    macro_cache copy_c(file_mngr, copy_file);
    constexpr context::id_index copy_id("COPYFILE");

    auto ids = std::make_shared<context::id_storage>();

    analyzing_context ctx = create_analyzing_context(opencode_file_name, ids);
    save_dependency(copy_c, parse_dependency(copy_file, ctx, processing::processing_kind::COPY));

    // try parsing the opencode now
    analyzer opencode(opencode_text, analyzer_options { opencode_file_loc, ctx });
    opencode.analyze();
    EXPECT_TRUE(opencode.diags().empty());

    macro_cache_key copy_key { processing::processing_kind::COPY, copy_id, {} };

    analyzing_context new_ctx = create_analyzing_context(opencode_file_name, ids);
    EXPECT_TRUE(copy_c.load_from_cache(copy_key, new_ctx));

    analyzing_context new_ctx_2 = create_analyzing_context(opencode_file_name, ids);
    document_change simple_change({ { 0, 4 }, { 0, 5 } }, "16");
    file_mngr.did_change_file(copy_file_loc, 0, std::span(&simple_change, 1));
    EXPECT_FALSE(copy_c.load_from_cache(copy_key, new_ctx_2));
}
