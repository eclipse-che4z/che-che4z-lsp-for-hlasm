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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "diagnostic.h"
#include "utils/task.h"
#include "workspaces/library.h"

namespace {
class library_mock : public hlasm_plugin::parser_library::workspaces::library
{
public:
    // Inherited via library
    MOCK_METHOD(hlasm_plugin::utils::task, refresh, (), (override));
    MOCK_METHOD(hlasm_plugin::utils::task, prefetch, (), (override));
    MOCK_METHOD(std::vector<std::string>, list_files, (), (override));
    MOCK_METHOD(const hlasm_plugin::utils::resource::resource_location&, get_location, (), (const, override));
    MOCK_METHOD(bool, has_file, (std::string_view, hlasm_plugin::utils::resource::resource_location* url), (override));
    MOCK_METHOD(void, copy_diagnostics, (std::vector<hlasm_plugin::parser_library::diagnostic>&), (const, override));
    MOCK_METHOD(bool, has_cached_content, (), (const, override));
};
} // namespace
