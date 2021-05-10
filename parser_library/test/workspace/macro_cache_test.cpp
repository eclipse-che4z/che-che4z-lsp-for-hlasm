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
    std::unordered_map<std::string, std::shared_ptr<file_with_text>> files_by_library_;

    std::function<bool(file_manager_cache_test_mock*,
        const std::string& library,
        analyzing_context ctx,
        const workspaces::library_data data)>
        parse_lib_cb;

    auto add_macro_or_copy(std::string file_name, std::string text)
    {
        auto file = std::make_shared<file_with_text>(file_name, text, *this);

        files_by_library_.emplace(file_name.substr(lib_prefix_length), file);
        files_by_fname_.emplace(std::move(file_name), file);
        return file;
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

    processor_file_ptr get_proc_file_from_library(const std::string& library) const
    {
        auto it = files_by_library_.find(library);
        return it == files_by_library_.end() ? nullptr : it->second;
    };



    /*workspaces::parse_result parse_library(
        const std::string& library, analyzing_context ctx, const workspaces::library_data data) override
    {
        auto file = get_proc_file_from_library(library);
        if (!file)
            return false;
        return file->parse_macro(*this, ctx, data);
    };*/

    workspaces::parse_result parse_library(
        const std::string& library, analyzing_context ctx, const workspaces::library_data data) override
    {
        return parse_lib_cb(this, library, ctx, data);
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
    analyzing_context new_ctx { std::make_shared<context::hlasm_context>(std::move(file_name), asm_option(), std::move(ids)),
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
    auto macro = file_mngr.add_macro_or_copy(macro_file_name, macro_text);
    auto copyfile = file_mngr.add_macro_or_copy(copyfile_file_name, copyfile_text);

    macro_cache macro_c(file_mngr, *macro);
    macro_cache copy_c(file_mngr, *copyfile);
    context::hlasm_ctx_ptr hlasm_ctx;
    file_mngr.parse_lib_cb = [&](file_manager_cache_test_mock* mngr,
                                 const std::string& library,
                                 analyzing_context ctx,
                                 const workspaces::library_data data) {
        auto m = mngr->get_proc_file_from_library(library);
        auto a =
            std::make_unique<analyzer>(m->get_text(), m->get_file_name(), ctx, *mngr, data);
        a->analyze();
        auto key = macro_cache_key::create_from_context(*ctx.hlasm_ctx, data);
        auto& cache = m == macro ? macro_c : copy_c;
        cache.save_analyzer(key, std::move(a));
        hlasm_ctx = ctx.hlasm_ctx;
        return true;
    };


    opencode->parse(file_mngr);
    opencode->collect_diags();
    EXPECT_EQ(opencode->diags().size(), 0U);

    auto macro_id = hlasm_ctx->ids().add("MAC");
    auto copy_id = hlasm_ctx->ids().add("COPYFILE");

    analyzing_context new_ctx = create_analyzing_context(opencode_file_name, hlasm_ctx->move_ids());


    macro_cache_key macro_key { { processing::processing_kind::MACRO, macro_id }, {} };

    
    EXPECT_TRUE(macro_c.load_from_cache(macro_key, new_ctx));

    EXPECT_NE(new_ctx.hlasm_ctx->get_macro_definition(macro_id), nullptr);
    EXPECT_NE(new_ctx.lsp_ctx->get_macro_info(macro_id), nullptr);
    EXPECT_NE(new_ctx.hlasm_ctx->get_copy_member(copy_id), nullptr);


    macro->did_change({}, " ");

    analyzing_context ctx_macro_changed = create_analyzing_context(opencode_file_name, new_ctx.hlasm_ctx->move_ids());

    macro_cache_key copy_key { { processing::processing_kind::COPY, copy_id }, {} };
    // After macro change, copy should still be cached
    EXPECT_TRUE(copy_c.load_from_cache(copy_key, ctx_macro_changed));
    EXPECT_NE(ctx_macro_changed.hlasm_ctx->get_copy_member(copy_id), nullptr);
    EXPECT_FALSE(macro_c.load_from_cache(macro_key, ctx_macro_changed));


    // Reparse the change so everything is cached again
    opencode->parse(file_mngr);

    copyfile->did_change({}, " ");

    analyzing_context ctx_copy_changed =
        create_analyzing_context(opencode_file_name, hlasm_ctx->move_ids());

    // Macro depends on the copyfile, so none should be cached.
    EXPECT_FALSE(macro_c.load_from_cache(macro_key, ctx_copy_changed));
    EXPECT_FALSE(copy_c.load_from_cache(copy_key, ctx_copy_changed));
}


TEST(macro_cache_test, macro_opsyn)
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
    // file_mngr.add_macro_or_copy()
    auto opencode = file_mngr.add_opencode(opencode_file_name, opencode_text);
    auto macro = file_mngr.add_macro_or_copy(macro_file_name, macro_text);

    macro_cache macro_c(file_mngr, *macro);
    context::hlasm_ctx_ptr hlasm_ctx;
    file_mngr.parse_lib_cb = [&](file_manager_cache_test_mock* mngr,
                                 const std::string& library,
                                 analyzing_context ctx,
                                 const workspaces::library_data data) {
        auto m = mngr->get_proc_file_from_library(library);
        auto a = std::make_unique<analyzer>(m->get_text(), m->get_file_name(), ctx, *mngr, data);
        a->analyze();
        auto key = macro_cache_key::create_from_context(*ctx.hlasm_ctx, data);
        macro_c.save_analyzer(key, std::move(a));
        hlasm_ctx = ctx.hlasm_ctx;
        return true;
    };

    opencode->parse(file_mngr);
    opencode->collect_diags();
    EXPECT_EQ(opencode->diags().size(), 0U);

    auto macro_id = hlasm_ctx->ids().add("MAC");

    analyzing_context new_ctx = create_analyzing_context(opencode_file_name, hlasm_ctx->move_ids());


    macro_cache_key macro_key { { processing::processing_kind::MACRO, macro_id }, {} };
    EXPECT_FALSE(macro_c.load_from_cache(macro_key, new_ctx));
}
