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

#ifndef HLASMPLUGIN_PARSERLIBRARY_LIBRARY_H
#define HLASMPLUGIN_PARSERLIBRARY_LIBRARY_H

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "diagnosable.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::parser_library::workspaces {

class processor;

class library
{
public:
    virtual ~library() = default;
    virtual void refresh() = 0;
    virtual std::vector<std::string> list_files() = 0;
    virtual std::string refresh_url_prefix() const = 0;
    virtual bool has_file(std::string_view file, utils::resource::resource_location* url = nullptr) = 0;
    virtual void copy_diagnostics(std::vector<diagnostic_s>&) const = 0;
};

} // namespace hlasm_plugin::parser_library::workspaces
#endif
