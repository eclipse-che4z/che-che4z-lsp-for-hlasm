/*
 * Copyright (c) 2024 Broadcom.
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

#include "output_handler.h"

class output_hanler_mock : public hlasm_plugin::parser_library::output_handler
{
public:
    MOCK_METHOD(void, mnote, (unsigned char level, std::string_view text), (override));
    MOCK_METHOD(void, punch, (std::string_view text), (override));
};
