/*
 * Copyright (c) 2023 Broadcom.
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

#include <optional>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "external_configuration_requests.h"

namespace {
class external_configuration_requests_mock : public hlasm_plugin::parser_library::external_configuration_requests
{
public:
    MOCK_METHOD(void,
        read_external_configuration,
        (hlasm_plugin::parser_library::sequence<char> url,
            hlasm_plugin::parser_library::workspace_manager_response<hlasm_plugin::parser_library::sequence<char>>
                content),
        (override));
};
} // namespace
