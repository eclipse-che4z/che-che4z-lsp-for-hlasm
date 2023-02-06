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

#include "library_info_transitional.h"

#include "context/hlasm_context.h"
#include "workspaces/parse_lib_provider.h"

namespace {
const hlasm_plugin::utils::resource::resource_location empty_location;
}

namespace hlasm_plugin::parser_library {

bool library_info_transitional::has_library(std::string_view member) const
{
    return m_lib_provider->has_library(member);
}

const library_info_transitional library_info_transitional::empty(workspaces::empty_parse_lib_provider::instance);
} // namespace hlasm_plugin::parser_library
