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

#include <string>
#include <string_view>
#include <vector>

namespace hlasm_plugin {
namespace parser_library {
struct diagnostic;
} // namespace parser_library
namespace utils::resource {
class resource_location;
} // namespace utils::resource
namespace utils {
class task;
} // namespace utils
} // namespace hlasm_plugin

namespace hlasm_plugin::parser_library::workspaces {

class processor;

class library
{
public:
    virtual ~library() = default;
    [[nodiscard]] virtual utils::task refresh() = 0;
    [[nodiscard]] virtual utils::task prefetch() = 0;
    virtual std::vector<std::string> list_files() = 0;
    virtual const utils::resource::resource_location& get_location() const = 0;
    virtual bool has_file(std::string_view file, utils::resource::resource_location* url = nullptr) = 0;
    virtual void copy_diagnostics(std::vector<diagnostic>&) const = 0;
    virtual bool has_cached_content() const = 0;
};

} // namespace hlasm_plugin::parser_library::workspaces
#endif
