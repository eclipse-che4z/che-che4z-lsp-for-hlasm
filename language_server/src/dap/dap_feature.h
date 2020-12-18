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

#ifndef HLASMPLUGIN_LANGUAGESERVER_DAP_DAP_FEATURE_H
#define HLASMPLUGIN_LANGUAGESERVER_DAP_DAP_FEATURE_H

#include <filesystem>

#include "../feature.h"

namespace hlasm_plugin::language_server::dap {

enum class path_format
{
    PATH,
    URI
};

// Implements DAP-specific capabilities that are needed for all features:
// path format (path vs URI) and zero-based vs 1-based line and column numbers
class dap_feature : public feature
{
public:
    void virtual initialize_feature(const json& client_capabilities) override
    {
        line_1_based_ = client_capabilities["linesStartAt1"].get<bool>() ? 1 : 0;
        column_1_based_ = client_capabilities["columnsStartAt1"].get<bool>() ? 1 : 0;
        path_format_ =
            client_capabilities["pathFormat"].get<std::string>() == "path" ? path_format::PATH : path_format::URI;
    }

protected:
    dap_feature(parser_library::workspace_manager& ws_mngr, response_provider& response_provider)
        : feature(ws_mngr, response_provider)
    {}

    std::string convert_path(const std::string& path) const
    {
        if (path_format_ == path_format::URI)
            return uri_to_path(path);

        std::filesystem::path p(path);
        // Theia sends us relative path (while not accepting it back) change, to absolute
        if (p.is_relative())
            p = std::filesystem::absolute(p);
        std::string result = p.lexically_normal().string();

        // on windows, VS code sends us path with capital drive letter through DAP and
        // lowercase drive letter through LSP.
        // Remove, once we implement case-insensitive comparison of paths in parser_library for windows
#ifdef _WIN32
        if (result[1] == ':')
            result[0] = (char)tolower(result[0]);
#endif //  _WIN32

        return result;
    }

    int column_1_based_;
    int line_1_based_;

private:
    path_format path_format_;
};



} // namespace hlasm_plugin::language_server::dap

#endif // !HLASMPLUGIN_LANGUAGESERVER_DAP_DAP_FEATURE_H
