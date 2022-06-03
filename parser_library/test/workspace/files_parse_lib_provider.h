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

#include "analyzer.h"
#include "workspaces/file_manager.h"

namespace hlasm_plugin::parser_library::workspaces {

struct files_parse_lib_provider : public workspaces::parse_lib_provider
{
    file_manager* file_mngr;
    files_parse_lib_provider(file_manager& mngr)
        : file_mngr(&mngr)
    {}
    virtual parse_result parse_library(const std::string& library, analyzing_context ctx, library_data data) override
    {
        auto macro = file_mngr->add_processor_file(utils::resource::resource_location(library));
        if (!macro)
            return false;
        return macro->parse_macro(*this, std::move(ctx), std::move(data));
    }
    virtual bool has_library(const std::string& library, const utils::resource::resource_location&) const override
    {
        return file_mngr->find(utils::resource::resource_location(library)) != nullptr;
    }
    virtual std::optional<std::string> get_library(const std::string& library,
        const utils::resource::resource_location&,
        std::optional<utils::resource::resource_location>& lib_location) const override
    {
        lib_location = std::nullopt;

        auto macro = file_mngr->add_processor_file(utils::resource::resource_location(library));
        if (!macro)
            return std::nullopt;
        return macro->get_text();
    }
};

} // namespace hlasm_plugin::parser_library::workspaces

#endif
