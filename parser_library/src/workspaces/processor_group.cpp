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

#include "processor_group.h"

#include "config/proc_grps.h"

namespace hlasm_plugin::parser_library::workspaces {

namespace {
struct translate_pp_options
{
    preprocessor_options operator()(const std::monostate&) const { return std::monostate {}; }
    preprocessor_options operator()(const config::db2_preprocessor& opt) const
    {
        return db2_preprocessor_options(opt.version);
    }
    preprocessor_options operator()(const config::cics_preprocessor& opt) const
    {
        return cics_preprocessor_options(opt.prolog, opt.epilog, opt.leasm);
    }
};
} // namespace

processor_group::processor_group(const std::string& pg_name,
    std::string_view pg_file_name,
    const config::assembler_options& asm_options,
    const config::preprocessor_options& pp)
    : m_pg_name(pg_name)
    , m_asm_opts(translate_assembler_options(asm_options, pg_file_name))
    , m_prep_opts(std::visit(translate_pp_options {}, pp.options))
{}

instruction_set_version processor_group::find_instruction_set(
    std::string_view optable, std::string_view pg_file_name) const
{
#ifdef __cpp_lib_ranges
    auto it = std::ranges::lower_bound(
        instr_set_version_equivalents, optable, {}, [](const auto& instr) { return instr.first; });
#else
    auto it = std::lower_bound(std::begin(instr_set_version_equivalents),
        std::end(instr_set_version_equivalents),
        optable,
        [](const auto& l, const auto& r) { return l.first < r; });
#endif

    if (it == std::end(instr_set_version_equivalents) || it->first != optable)
    {
        add_diagnostic(diagnostic_s::error_W0007(pg_file_name, m_pg_name));
        return asm_option::instr_set_default;
    }

    return it->second;
}

asm_option processor_group::translate_assembler_options(
    const config::assembler_options& asm_options, std::string_view pg_file_name) const
{
    return asm_option { .sysparm = asm_options.sysparm,
        .profile = asm_options.profile,
        .instr_set = asm_options.optable.empty() ? asm_option::instr_set_default
                                                 : find_instruction_set(asm_options.optable, pg_file_name),
        .system_id = asm_options.system_id.empty() ? asm_option::system_id_default : asm_options.system_id,
        .sysopt_xobject = asm_options.goff };
}

void processor_group::collect_diags() const
{
    for (auto&& lib : m_libs)
    {
        collect_diags_from_child(*lib);
    }
}

void processor_group::add_library(std::unique_ptr<library> library) { m_libs.push_back(std::move(library)); }

} // namespace hlasm_plugin::parser_library::workspaces
