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
#ifndef HLASMPLUGIN_PARSERLIBRARY_TEST_FILES_PARSE_LIB_PROVIDER_H
#define HLASMPLUGIN_PARSERLIBRARY_TEST_FILES_PARSE_LIB_PROVIDER_H

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "analyzer.h"
#include "workspaces/file_manager.h"
#include "workspaces/workspace.h"

namespace hlasm_plugin::parser_library::workspaces {

struct files_parse_lib_provider : public workspaces::parse_lib_provider
{
    file_manager* file_mngr;
    workspace* ws;
    files_parse_lib_provider(file_manager& mngr, workspace& ws)
        : file_mngr(&mngr)
        , ws(&ws)
    {}
    parse_result parse_library(const std::string& library, analyzing_context ctx, library_data data) override
    {
        auto macro = ws->add_processor_file(utils::resource::resource_location(library));
        if (!macro)
            return false;
        return macro->parse_macro(*this, std::move(ctx), std::move(data));
    }
    bool has_library(const std::string& library, const utils::resource::resource_location&) const override
    {
        return file_mngr->find(utils::resource::resource_location(library)) != nullptr;
    }
    std::optional<std::pair<std::string, utils::resource::resource_location>> get_library(
        const std::string& library, const utils::resource::resource_location&) const override
    {
        auto macro = file_mngr->find(utils::resource::resource_location(library));
        if (!macro)
            return std::nullopt;
        return std::pair<std::string, hlasm_plugin::utils::resource::resource_location>(
            macro->get_text(), macro->get_location());
    }
};

} // namespace hlasm_plugin::parser_library::workspaces

#endif
