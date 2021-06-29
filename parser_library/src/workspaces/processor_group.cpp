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

#include "config/proc_conf.h"

namespace hlasm_plugin::parser_library::workspaces {

namespace {
struct translate_pp_options
{
    preprocessor_options operator()(const std::monostate&) { return std::monostate {}; }
    preprocessor_options operator()(const config::db2_preprocessor&) { return db2_preprocessor_options {}; }
};
} // namespace

processor_group::processor_group(
    const std::string& name, const config::assembler_options& asm_options, const config::preprocessor_options& pp)
    : name_(name)
    , asm_optns { asm_options.sysparm, asm_options.profile }
    , prep_opts(std::visit(translate_pp_options {}, pp.options))
{}

void processor_group::collect_diags() const
{
    for (auto&& lib : libs_)
    {
        collect_diags_from_child(*lib);
    }
}

void processor_group::add_library(std::unique_ptr<library> library) { libs_.push_back(std::move(library)); }

} // namespace hlasm_plugin::parser_library::workspaces
