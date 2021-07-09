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

#include "analyzer.h"

constexpr const char* MACRO_FILE = "MAC";
constexpr const char* SOURCE_FILE = "OPEN";
constexpr const char* COPY_FILE = "path/COPYFILE";

namespace hlasm_plugin::parser_library {

class mock_parse_lib_provider : public workspaces::parse_lib_provider
{
public:
    workspaces::parse_result parse_library(
        const std::string&, analyzing_context ctx, workspaces::library_data data) override
    {
        analyzer a(data.proc_kind == processing::processing_kind::MACRO ? macro_contents : copy_contents,
            analyzer_options {
                data.proc_kind == processing::processing_kind::MACRO ? MACRO_FILE : COPY_FILE,
                this,
                std::move(ctx),
                data,
            });

        a.analyze();
        return true;
    }
    bool has_library(const std::string&, const std::string&) const override { return true; }
    std::optional<std::string> get_library(
        const std::string& library, const std::string& program, std::string*) const override
    {
        if (library == "MAC")
            return macro_contents;
        else if (library == "COPYFILE")
            return copy_contents;
        else
            return std::nullopt;
    }

private:
    const std::string macro_contents =
        R"(   MACRO
       MAC   &VAR
       LR    &VAR,&VAR
       MEND
)";
    const std::string copy_contents =
        R"(R2 EQU 2
            LR R2,R2)";
};

} // namespace hlasm_plugin::parser_library