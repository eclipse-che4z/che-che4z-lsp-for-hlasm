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

#ifndef HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_GROUP_H
#define HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_GROUP_H

#include "compiler_options.h"
#include "diagnosable_impl.h"
#include "library.h"
#include "preprocessor_options.h"

namespace hlasm_plugin::parser_library::config {
struct assembler_options;
struct preprocessor_options;
} // namespace hlasm_plugin::parser_library::config

namespace hlasm_plugin::parser_library::workspaces {

// Represents a named set of libraries (processor_group)
class processor_group : public diagnosable_impl
{
public:
    processor_group(
        const std::string& name, const config::assembler_options& asm_options, const config::preprocessor_options& pp);

    void collect_diags() const override;

    void add_library(std::unique_ptr<library> library);

    const std::string& name() const { return name_; }

    const std::vector<std::unique_ptr<library>>& libraries() const { return libs_; }

    const asm_option& asm_options() const { return asm_optns; }

    const preprocessor_options& preprocessor() const { return prep_opts; }

private:
    std::vector<std::unique_ptr<library>> libs_;
    std::string name_;
    asm_option asm_optns;
    preprocessor_options prep_opts;
};
} // namespace hlasm_plugin::parser_library::workspaces
#endif // !HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_GROUP_H
