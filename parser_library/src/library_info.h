/*
 * Copyright (c) 2022 Broadcom.
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

#ifndef HLASMPLUGIN_PARSERLIBRARY_LIBRARY_INFO_H
#define HLASMPLUGIN_PARSERLIBRARY_LIBRARY_INFO_H

#include <string_view>

namespace hlasm_plugin::parser_library {

class library_info
{
public:
    virtual bool has_library(std::string_view member) const = 0;

protected:
    ~library_info() = default;
};

} // namespace hlasm_plugin::parser_library

#endif
