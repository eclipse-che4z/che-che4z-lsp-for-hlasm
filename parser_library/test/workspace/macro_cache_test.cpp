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
#include <fstream>
#include <iterator>

#include "gtest/gtest.h"

#include "analyzer.h"
#include "file_with_text.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/processor_file_impl.h"


using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::workspaces;

struct file_manager_cache_test_mock : public file_manager_impl, public parse_lib_provider
{
    const static inline size_t lib_prefix_length = 4;

    std::unordered_map<std::string, std::shared_ptr<file_with_text>> files_by_fname_;
    std::unordered_map<std::string, std::pair<std::shared_ptr<file_with_text>, macro_cache>> files_by_library_;

    context::hlasm_ctx_ptr hlasm_ctx;

    auto& add_macro_or_copy(std::string file_name, std::string text)
    {
        auto file = std::make_shared<file_with_text>(file_name, text, *this);

        auto [it, succ] = files_by_library_.emplace(file_name.substr(lib_prefix_length),
            std::pair<std::shared_ptr<file_with_text>, macro_cache>(
                std::piecewise_construct, std::forward_as_tuple(file), std::forward_as_tuple(*this, *file)));
        files_by_fname_.emplace(std::move(file_name), file);

        return it->second;
    }

    auto add_opencode(std::string file_name, std::string text)
    {
        auto file = std::make_shared<file_with_text>(file_name, text, *this);
        files_by_fname_.emplace(std::move(file_name), file);
        return file;
    }


    file_ptr find(const std::string& key) const override
    {
        auto it = files_by_fname_.find(key);
        return it == files_by_fname_.end() ? nullptr : it->second;
    };

    std::pair<processor_file_ptr, macro_cache*> get_proc_file_from_library(const std::string& library)
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
        auto a = std::make_unique<analyzer>(m->get_text(), m->get_file_name(), ctx, *this, data);
        a->analyze();
        auto key = macro_cache_key::create_from_context(*ctx.hlasm_ctx, data);
        cache->save_analyzer(key, std::move(a));
        hlasm_ctx = ctx.hlasm_ctx;
        return true;
    }

    bool has_library(const std::string& library, const std::string&) const override
    {
        return files_by_library_.count(library) > 0;
    };

    asm_option empty_options;
    const asm_option& get_asm_options(const std::string&) override { return empty_options; };
};

analyzing_context create_analyzing_context(std::string file_name, context::id_storage ids)
{
    analyzing_context new_ctx { std::make_shared<context::hlasm_context>(
                                    std::move(file_name), asm_option(), std::move(ids)),
        std::make_shared<lsp::lsp_context>() };
    lsp::opencode_info_ptr oip =
        std::make_unique<lsp::opencode_info>(*new_ctx.hlasm_ctx, lsp::vardef_storage(), lsp::file_occurences_t {});
    new_ctx.lsp_ctx->add_opencode(std::move(oip), lsp::text_data_ref_t(""));

    return new_ctx;
}

TEST(macro_cache_test, copy_from_macro)
{
    std::string opencode_file_name = "opencode";
    std::string opencode_text =
        R"(
       MAC 1

)";
    std::string macro_file_name = "lib/MAC";
    std::string macro_text =
        R"( MACRO
       MAC &PARAM
       COPY COPYFILE
       MEND
)";
    std::string copyfile_file_name = "lib/COPYFILE";
    std::string copyfile_text =
        R"(
       LR 15,1
)";

    file_manager_cache_test_mock file_mngr;
    // file_mngr.add_macro_or_copy()
    auto opencode = file_mngr.add_opencode(opencode_file_name, opencode_text);
    auto& [macro, macro_c] = file_mngr.add_macro_or_copy(macro_file_name, macro_text);
    auto& [copyfile, copy_c] = file_mngr.add_macro_or_copy(copyfile_file_name, copyfile_text);


    opencode->parse(file_mngr);
    opencode->collect_diags();
    EXPECT_EQ(opencode->diags().size(), 0U);

    auto macro_id = file_mngr.hlasm_ctx->ids().add("MAC");
    auto copy_id = file_mngr.hlasm_ctx->ids().add("COPYFILE");

    analyzing_context new_ctx = create_analyzing_context(opencode_file_name, file_mngr.hlasm_ctx->ids());


    macro_cache_key macro_key { opencode_file_name, { processing::processing_kind::MACRO, macro_id }, {} };


    EXPECT_TRUE(macro_c.load_from_cache(macro_key, new_ctx));

    EXPECT_NE(new_ctx.hlasm_ctx->get_macro_definition(macro_id), nullptr);
    EXPECT_NE(new_ctx.lsp_ctx->get_macro_info(macro_id), nullptr);
    EXPECT_NE(new_ctx.hlasm_ctx->get_copy_member(copy_id), nullptr);


    macro->did_change({}, " ");

    analyzing_context ctx_macro_changed = create_analyzing_context(opencode_file_name, new_ctx.hlasm_ctx->ids());

    macro_cache_key copy_key { opencode_file_name, { processing::processing_kind::COPY, copy_id }, {} };
    // After macro change, copy should still be cached
    EXPECT_TRUE(copy_c.load_from_cache(copy_key, ctx_macro_changed));
    EXPECT_NE(ctx_macro_changed.hlasm_ctx->get_copy_member(copy_id), nullptr);
    EXPECT_FALSE(macro_c.load_from_cache(macro_key, ctx_macro_changed));


    // Reparse the change so everything is cached again
    opencode->parse(file_mngr);

    copyfile->did_change({}, " ");

    analyzing_context ctx_copy_changed = create_analyzing_context(opencode_file_name, file_mngr.hlasm_ctx->ids());

    // Macro depends on the copyfile, so none should be cached.
    EXPECT_FALSE(macro_c.load_from_cache(macro_key, ctx_copy_changed));
    EXPECT_FALSE(copy_c.load_from_cache(copy_key, ctx_copy_changed));
}

TEST(macro_cache_test, opsyn_change)
{
    std::string opencode_file_name = "opencode";
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
    auto opencode = file_mngr.add_opencode(opencode_file_name, opencode_text);
    auto& [macro, macro_c] = file_mngr.add_macro_or_copy(macro_file_name, macro_text);

    opencode->parse(file_mngr);
    opencode->collect_diags();
    EXPECT_EQ(opencode->diags().size(), 0U);


    auto macro_id = file_mngr.hlasm_ctx->ids().add("MAC");
    auto& ids = file_mngr.hlasm_ctx->ids();

    macro_cache_key macro_key_one_opsyn { opencode_file_name,
        { processing::processing_kind::MACRO, macro_id },
        { cached_opsyn_mnemo { ids.well_known.SETA, ids.add("LR"), false } } };


    analyzing_context new_ctx = create_analyzing_context(opencode_file_name, ids);


    macro_cache_key macro_key { opencode_file_name, { processing::processing_kind::MACRO, macro_id }, {} };
    EXPECT_FALSE(macro_c.load_from_cache(macro_key, new_ctx));
    EXPECT_TRUE(macro_c.load_from_cache(macro_key_one_opsyn, new_ctx));



    opencode->did_change({}, "L OPSYN SETB\n");
    opencode->parse(file_mngr);

    analyzing_context ctx_second_opsyn1 = create_analyzing_context(opencode_file_name, file_mngr.hlasm_ctx->ids());
    EXPECT_TRUE(macro_c.load_from_cache(macro_key_one_opsyn, ctx_second_opsyn1));


    macro_cache_key macro_key_two_opsyns = macro_key_one_opsyn;
    macro_key_two_opsyns.opsyn_state.push_back(
        cached_opsyn_mnemo { file_mngr.hlasm_ctx->ids().add("L"), file_mngr.hlasm_ctx->ids().well_known.SETB, false });

    macro_cache_key::sort_opsyn_state(macro_key_two_opsyns.opsyn_state);

    analyzing_context ctx_second_opsyn2 = create_analyzing_context(opencode_file_name, file_mngr.hlasm_ctx->ids());
    EXPECT_TRUE(macro_c.load_from_cache(macro_key_two_opsyns, ctx_second_opsyn2));
}

TEST(macro_cache_test, empty_macro)
{
    std::string opencode_file_name = "opencode";
    // This tests a caveat where parse_library is called twice for the same macro, if the macro is not defined in its
    // file.
    std::string opencode_text = R"(
 MAC
 MAC)";
    std::string macro_file_name = "lib/MAC";
    std::string macro_text = "";

    file_manager_cache_test_mock file_mngr;
    auto opencode = file_mngr.add_opencode(opencode_file_name, opencode_text);
    auto& [macro, macro_c] = file_mngr.add_macro_or_copy(macro_file_name, macro_text);

    opencode->parse(file_mngr);

    auto macro_id = file_mngr.hlasm_ctx->ids().add("MAC");

    analyzing_context new_ctx = create_analyzing_context(opencode_file_name, file_mngr.hlasm_ctx->ids());

    macro_cache_key macro_key { opencode_file_name, { processing::processing_kind::MACRO, macro_id }, {} };
    EXPECT_TRUE(macro_c.load_from_cache(macro_key, new_ctx));
    EXPECT_EQ(new_ctx.hlasm_ctx->macros().count(macro_id), 0U);
}

TEST(macro_cache_test, get_opsyn_state)
{
    std::string opencode_file_name = "opencode";
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

    auto& ids = a.hlasm_ctx().ids();

    std::vector<cached_opsyn_mnemo> expected {
        { ids.well_known.SETA, ids.add("LR"), false },
        { ids.add("L"), ids.well_known.SETB, false },
        { ids.well_known.SETC, ids.well_known.SETC, true },
        { ids.add("MAC"), ids.add("AREAD"), false },
    };

    macro_cache_key::sort_opsyn_state(expected);

    EXPECT_EQ(state, expected);
}

TEST(macro_cache_test, overwrite_by_inline)
{
    std::string opencode_file_name = "opencode";
    std::string opencode_text =
        R"(
       MAC

       MACRO
       MAC
       LR 1,16
       MEND
       
       MAC
)";
    std::string macro_file_name = "lib/MAC";
    std::string macro_text =
        R"( MACRO
       MAC
       LR 1,16
       MEND
)";

    file_manager_cache_test_mock file_mngr;
    auto opencode = file_mngr.add_opencode(opencode_file_name, opencode_text);
    auto& [macro, macro_c] = file_mngr.add_macro_or_copy(macro_file_name, macro_text);

    opencode->parse(file_mngr);
    opencode->collect_diags();
    EXPECT_EQ(opencode->diags().size(), 2U);


    auto macro_diag = std::find_if(opencode->diags().begin(), opencode->diags().end(), [&](const diagnostic_s& d) {
        return d.file_name == macro_file_name;
    });
    EXPECT_NE(macro_diag, opencode->diags().end());

    auto opencode_diag = std::find_if(opencode->diags().begin(), opencode->diags().end(), [&](const diagnostic_s& d) {
        return d.file_name == opencode_file_name;
    });
    EXPECT_NE(opencode_diag, opencode->diags().end());
}
