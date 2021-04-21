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
    const static inline std::string opencode_file_name = "opencode";
    const static inline std::string opencode_text =
        R"(
       MAC 1

       COPY COPYFILE
       LR &VAR,1
       L 1,SYM

       COPY NOTEXIST
)";
    const static inline std::string macro_file_name = "lib/MAC";
    const static inline std::string macro_text =
        R"( MACRO
       MAC &PARAM

       COPY COPYFILE

SYM    LR &VAR,1

       MEND
)";
    const static inline std::string copyfile_file_name = "lib/COPYFILE";
    const static inline std::string copyfile_text =
        R"(
       
       LR &PARAM,1
&VAR   SETA 1

)";
    const static inline size_t lib_prefix_length = 4;

    std::shared_ptr<file_with_text> opencode;
    std::shared_ptr<file_with_text> macro;
    std::shared_ptr<file_with_text> copyfile;

    file_ptr find(const std::string& key) const override
    {
        if (key.substr(lib_prefix_length) == opencode_file_name)
            return opencode;
        else if (key.substr(lib_prefix_length) == macro_file_name)
            return macro;
        else if (key.substr(lib_prefix_length) == copyfile_file_name)
            return copyfile;
        else
            return nullptr;
    };



    workspaces::parse_result parse_library(
        const std::string& library, analyzing_context ctx, const workspaces::library_data data) override
    {
        auto file = find(library);
        auto proc_file_ptr = dynamic_cast<processor_file_impl*>(file.get());
        return proc_file_ptr->parse_macro(*this, ctx, data);
    };

    bool has_library(const std::string& library, const std::string&) const override
    {
        return library.substr(lib_prefix_length) == copyfile_file_name
            || library.substr(lib_prefix_length) == macro_file_name;
    };

    asm_option empty_options;
    const asm_option& get_asm_options(const std::string&) override { return empty_options; };

    file_manager_cache_test_mock()
    {
        opencode = std::make_shared<file_with_text>(opencode_file_name, opencode_text, *this);
        macro = std::make_shared<file_with_text>(macro_file_name, macro_text, *this);
        copyfile = std::make_shared<file_with_text>(copyfile_file_name, copyfile_text, *this);
    }
};



TEST(macro_cache_test, testtest)
{
    file_manager_cache_test_mock file_mngr;
    file_mngr.opencode->parse(file_mngr);

}
