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

#include "gtest/gtest.h"

#include "../workspace/empty_configs.h"
#include "analyzer.h"
#include "context/id_storage.h"
#include "files_parse_lib_provider.h"
#include "utils/resource_location.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/processor_file_impl.h"
#include "workspaces/workspace.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::utils::resource;

namespace {

struct file_manager_cache_test_mock : public file_manager_impl, public parse_lib_provider
{
    const static inline size_t lib_prefix_length = 4;

    std::unordered_map<resource_location, std::shared_ptr<file>, resource_location_hasher> files_by_location_;
    std::unordered_map<std::string, std::pair<std::shared_ptr<file>, macro_cache>> files_by_library_;

    std::shared_ptr<context::hlasm_context> hlasm_ctx;

    auto& add_macro_or_copy(std::string file_name, std::string text)
    {
        auto file_loc = resource_location(file_name);

        did_open_file(file_loc, {}, text);
        auto f = file_manager_impl::find(file_loc);

        auto [it, succ] = files_by_library_.try_emplace(file_name.substr(lib_prefix_length), f, macro_cache(*this, *f));
        files_by_location_.emplace(std::move(file_loc), f);

        return it->second;
    }

    auto add_opencode(std::string file_name, std::string text)
    {
        auto file_loc = resource_location(file_name);

        did_open_file(file_loc, {}, text);
        auto f = file_manager_impl::find(file_loc);
        files_by_location_.emplace(std::move(file_loc), f);
        return f;
    }


    std::shared_ptr<file> find(const resource_location& key) const override
    {
        auto it = files_by_location_.find(key);
        return it == files_by_location_.end() ? nullptr : it->second;
    };

    std::pair<std::shared_ptr<file>, macro_cache*> get_proc_file_from_library(const std::string& library)
    {
        auto it = files_by_library_.find(library);
        if (it == files_by_library_.end())
            return { nullptr, nullptr };
        else
            return { it->second.first, &it->second.second };
    };

    workspaces::parse_result parse_library(
        const std::string& library, analyzing_context ctx, const workspaces::library_data data) override
    {
        auto [m, cache] = get_proc_file_from_library(library);
        auto a = std::make_unique<analyzer>(m->get_text(), analyzer_options { m->get_location(), ctx, this, data });
        a->analyze();
        auto key = macro_cache_key::create_from_context(*ctx.hlasm_ctx, data);
        cache->save_macro(key, *a);
        hlasm_ctx = ctx.hlasm_ctx;
        return true;
    }

    bool has_library(const std::string& library, const resource_location&) const override
    {
        return files_by_library_.count(library) > 0;
    };

    std::optional<std::pair<std::string, resource_location>> get_library(
        const std::string& library, const resource_location&) const override
    {
        auto it = files_by_library_.find(library);
        if (it == files_by_library_.end())
            return std::nullopt;
        return std::pair<std::string, resource_location>(
            it->second.first->get_text(), it->second.first->get_location());
    }
};

analyzing_context create_analyzing_context(std::string file_name, std::shared_ptr<context::id_storage> ids)
{
    auto hlasm_ctx =
        std::make_shared<context::hlasm_context>(resource_location(file_name), asm_option(), std::move(ids));
    analyzing_context new_ctx {
        hlasm_ctx,
        std::make_shared<lsp::lsp_context>(hlasm_ctx),
    };
    lsp::opencode_info_ptr oip = std::make_unique<lsp::opencode_info>(lsp::vardef_storage(), lsp::file_occurences_t {});
    new_ctx.lsp_ctx->add_opencode(std::move(oip), lsp::text_data_view(""));

    return new_ctx;
}

} // namespace

TEST(macro_cache_test, copy_from_macro)
{
    std::string opencode_file_name = "opencode";
    auto opencode_file_loc = resource_location(opencode_file_name);
    std::string opencode_text =
        R"(
       MAC 1

)";
    std::string macro_file_name = "lib/MAC";
    auto macro_file_loc = resource_location(macro_file_name);
    std::string macro_text =
        R"( MACRO
       MAC &PARAM
       COPY COPYFILE
       MEND
)";
    std::string copyfile_file_name = "lib/COPYFILE";
    auto copyfile_file_loc = resource_location(copyfile_file_name);
    std::string copyfile_text =
        R"(
       LR 15,1
)";

    file_manager_cache_test_mock file_mngr;
    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws(file_mngr, config, global_settings);
    ws.open();

    file_mngr.add_opencode(opencode_file_name, opencode_text);
    auto& [macro, macro_c] = file_mngr.add_macro_or_copy(macro_file_name, macro_text);
    auto& [copyfile, copy_c] = file_mngr.add_macro_or_copy(copyfile_file_name, copyfile_text);

    auto opencode = ws.add_processor_file(opencode_file_loc);


    opencode->parse(file_mngr, {}, {}, nullptr);
    opencode->collect_diags();
    EXPECT_EQ(opencode->diags().size(), 0U);

    auto macro_id = context::id_index("MAC");
    auto copy_id = context::id_index("COPYFILE");

    analyzing_context new_ctx = create_analyzing_context(opencode_file_name, file_mngr.hlasm_ctx->ids_ptr());


    macro_cache_key macro_key { opencode_file_loc, { processing::processing_kind::MACRO, macro_id }, {} };


    EXPECT_TRUE(macro_c.load_from_cache(macro_key, new_ctx));

    EXPECT_NE(new_ctx.hlasm_ctx->get_macro_definition(macro_id), nullptr);
    EXPECT_NE(new_ctx.lsp_ctx->get_macro_info(macro_id), nullptr);
    EXPECT_NE(new_ctx.hlasm_ctx->get_copy_member(copy_id), nullptr);


    macro->did_change({}, " ");

    analyzing_context ctx_macro_changed = create_analyzing_context(opencode_file_name, new_ctx.hlasm_ctx->ids_ptr());

    macro_cache_key copy_key { opencode_file_loc, { processing::processing_kind::COPY, copy_id }, {} };
    // After macro change, copy should still be cached
    EXPECT_TRUE(copy_c.load_from_cache(copy_key, ctx_macro_changed));
    EXPECT_NE(ctx_macro_changed.hlasm_ctx->get_copy_member(copy_id), nullptr);
    EXPECT_FALSE(macro_c.load_from_cache(macro_key, ctx_macro_changed));


    // Reparse the change so everything is cached again
    opencode->parse(file_mngr, {}, {}, nullptr);

    copyfile->did_change({}, " ");

    analyzing_context ctx_copy_changed = create_analyzing_context(opencode_file_name, file_mngr.hlasm_ctx->ids_ptr());

    // Macro depends on the copyfile, so none should be cached.
    EXPECT_FALSE(macro_c.load_from_cache(macro_key, ctx_copy_changed));
    EXPECT_FALSE(copy_c.load_from_cache(copy_key, ctx_copy_changed));
}

TEST(macro_cache_test, opsyn_change)
{
    std::string opencode_file_name = "opencode";
    auto opencode_file_loc = resource_location("opencode");
    std::string opencode_text =
        R"(
SETA   OPSYN LR
       MAC 1
)";
    std::string macro_file_name = "lib/MAC";
    std::string macro_text =
        R"( MACRO
       MAC &PARAM
       MEND
)";

    file_manager_cache_test_mock file_mngr;
    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws(file_mngr, config, global_settings);
    ws.open();
    auto opencode_file = file_mngr.add_opencode(opencode_file_name, opencode_text);
    auto& [macro, macro_c] = file_mngr.add_macro_or_copy(macro_file_name, macro_text);

    auto opencode = ws.add_processor_file(opencode_file_loc);

    opencode->parse(file_mngr, {}, {}, nullptr);
    opencode->collect_diags();
    EXPECT_EQ(opencode->diags().size(), 0U);


    auto macro_id = context::id_index("MAC");
    auto ids = file_mngr.hlasm_ctx->ids_ptr();

    constexpr context::id_index LR("LR");

    macro_cache_key macro_key_one_opsyn { opencode_file_loc,
        { processing::processing_kind::MACRO, macro_id },
        { cached_opsyn_mnemo { context::id_storage::well_known::SETA, LR, false } } };


    analyzing_context new_ctx = create_analyzing_context(opencode_file_name, ids);


    macro_cache_key macro_key { opencode_file_loc, { processing::processing_kind::MACRO, macro_id }, {} };
    EXPECT_FALSE(macro_c.load_from_cache(macro_key, new_ctx));
    EXPECT_TRUE(macro_c.load_from_cache(macro_key_one_opsyn, new_ctx));



    opencode_file->did_change({}, "L OPSYN SETB\n");
    EXPECT_EQ(opencode, ws.add_processor_file(opencode_file_loc));
    opencode->parse(file_mngr, {}, {}, nullptr);

    analyzing_context ctx_second_opsyn1 = create_analyzing_context(opencode_file_name, file_mngr.hlasm_ctx->ids_ptr());
    EXPECT_TRUE(macro_c.load_from_cache(macro_key_one_opsyn, ctx_second_opsyn1));

    constexpr context::id_index L("L");

    macro_cache_key macro_key_two_opsyns = macro_key_one_opsyn;
    macro_key_two_opsyns.opsyn_state.push_back(cached_opsyn_mnemo { L, context::id_storage::well_known::SETB, false });

    macro_cache_key::sort_opsyn_state(macro_key_two_opsyns.opsyn_state);

    analyzing_context ctx_second_opsyn2 = create_analyzing_context(opencode_file_name, file_mngr.hlasm_ctx->ids_ptr());
    EXPECT_TRUE(macro_c.load_from_cache(macro_key_two_opsyns, ctx_second_opsyn2));
}

TEST(macro_cache_test, empty_macro)
{
    std::string opencode_file_name = "opencode";
    auto opencode_file_loc = resource_location("opencode");

    // This tests a caveat where parse_library is called twice for the same macro, if the macro is not defined in its
    // file.
    std::string opencode_text = R"(
 MAC
 MAC)";
    std::string macro_file_name = "lib/MAC";
    std::string macro_text = "";

    file_manager_cache_test_mock file_mngr;
    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws(file_mngr, config, global_settings);
    ws.open();

    file_mngr.add_opencode(opencode_file_name, opencode_text);
    auto& [macro, macro_c] = file_mngr.add_macro_or_copy(macro_file_name, macro_text);
    auto opencode = ws.add_processor_file(opencode_file_loc);

    opencode->parse(file_mngr, {}, {}, nullptr);

    auto macro_id = context::id_index("MAC");

    analyzing_context new_ctx = create_analyzing_context(opencode_file_name, file_mngr.hlasm_ctx->ids_ptr());

    macro_cache_key macro_key { opencode_file_loc, { processing::processing_kind::MACRO, macro_id }, {} };
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
        { context::id_storage::well_known::SETA, LR, false },
        { L, context::id_storage::well_known::SETB, false },
        { context::id_storage::well_known::SETC, context::id_storage::well_known::SETC, true },
        { MAC, context::id_storage::well_known::AREAD, false },
    };

    macro_cache_key::sort_opsyn_state(expected);

    EXPECT_EQ(state, expected);
}

namespace {
std::optional<diagnostic_s> find_diag_with_filename(
    const std::vector<diagnostic_s>& diags, const resource_location& file)
{
    auto macro_diag =
        std::find_if(diags.begin(), diags.end(), [&](const diagnostic_s& d) { return d.file_uri == file.get_uri(); });
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
    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws(file_mngr, config, global_settings);
    ws.open();
    files_parse_lib_provider lib_provider(file_mngr, ws);

    file_mngr.did_open_file(opencode_file_loc, 0, opencode_text);
    file_mngr.did_open_file(macro_file_loc, 0, macro_text);
    auto opencode = ws.add_processor_file(opencode_file_loc);

    opencode->parse(lib_provider, {}, {}, nullptr);
    opencode->collect_diags();

    EXPECT_EQ(opencode->diags().size(), 2U);
    EXPECT_TRUE(find_diag_with_filename(opencode->diags(), macro_file_loc));
    EXPECT_TRUE(find_diag_with_filename(opencode->diags(), opencode_file_loc));

    opencode->diags().clear();

    opencode->parse(lib_provider, {}, {}, nullptr);
    opencode->collect_diags();
    EXPECT_EQ(opencode->diags().size(), 2U);
    EXPECT_TRUE(find_diag_with_filename(opencode->diags(), macro_file_loc));
    EXPECT_TRUE(find_diag_with_filename(opencode->diags(), opencode_file_loc));
}

TEST(macro_cache_test, inline_depends_on_copy)
{
    std::string opencode_file_name = "opencode";
    auto opencode_file_loc = resource_location("opencode");
    std::string opencode_text =
        R"(
       MACRO 
       MAC
       COPY COPYFILE
       MEND

       MAC
)";
    std::string copy_file_name = "lib/COPYFILE";
    std::string copy_text = R"( LR 1,1 arbitrary instruction)";

    file_manager_cache_test_mock file_mngr;
    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws(file_mngr, config, global_settings);
    ws.open();
    file_mngr.add_opencode(opencode_file_name, opencode_text);
    auto& [copyfile, copy_c] = file_mngr.add_macro_or_copy(copy_file_name, copy_text);
    auto opencode = ws.add_processor_file(opencode_file_loc);

    opencode->parse(file_mngr, {}, {}, nullptr);
    opencode->collect_diags();
    EXPECT_EQ(opencode->diags().size(), 0U);


    auto copy_id = context::id_index("COPYFILE");

    analyzing_context new_ctx = create_analyzing_context(opencode_file_name, file_mngr.hlasm_ctx->ids_ptr());


    macro_cache_key copy_key { opencode_file_loc, { processing::processing_kind::COPY, copy_id }, {} };
    EXPECT_TRUE(copy_c.load_from_cache(copy_key, new_ctx));

    copyfile->did_change({ { 0, 4 }, { 0, 5 } }, "16");
    opencode->parse(file_mngr, {}, {}, nullptr);
    opencode->collect_diags();
    ASSERT_EQ(opencode->diags().size(), 1U);
    EXPECT_EQ(opencode->diags()[0].code, "M120");
}
