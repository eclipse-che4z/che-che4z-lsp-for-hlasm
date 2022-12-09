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

#ifndef HLASMPLUGIN_PARSERLIBRARY_DEBUG_LIB_PROVIDER_H
#define HLASMPLUGIN_PARSERLIBRARY_DEBUG_LIB_PROVIDER_H

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "analyzing_context.h"
#include "workspaces/parse_lib_provider.h"
#include "workspaces/processor.h"
#include "workspaces/workspace.h"

namespace hlasm_plugin::parser_library::debugging {

// Implements dependency (macro and COPY files) fetcher for macro tracer.
// Takes the information from a workspace, but calls special methods for
// parsing that do not collide with LSP.
class debug_lib_provider final : public workspaces::parse_lib_provider
{
    const workspaces::workspace& ws_;

public:
    debug_lib_provider(const workspaces::workspace& ws)
        : ws_(ws)
    {}

    workspaces::parse_result parse_library(
        const std::string& library, analyzing_context ctx, workspaces::library_data data) override
    {
        auto& proc_grp = ws_.get_proc_grp_by_program(ctx.hlasm_ctx->opencode_location());
        for (auto&& lib : proc_grp.libraries())
        {
            std::shared_ptr<workspaces::processor> found = lib->find_file(library);
            if (found)
                return found->parse_no_lsp_update(*this, std::move(ctx), data);
        }

        return false;
    }

    bool has_library(const std::string& library, const utils::resource::resource_location& program) const override
    {
        return ws_.has_library(library, program);
    }

    std::optional<std::pair<std::string, utils::resource::resource_location>> get_library(
        const std::string& library, const utils::resource::resource_location& program) const override
    {
        return ws_.get_library(library, program);
    }
};

} // namespace hlasm_plugin::parser_library::debugging

#endif
