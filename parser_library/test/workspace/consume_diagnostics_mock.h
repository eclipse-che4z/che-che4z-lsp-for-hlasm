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

#include "gtest/gtest.h"

#include "workspace_manager.h"

using namespace hlasm_plugin::parser_library;

class diag_consumer_mock : public diagnostics_consumer
{
public:
    // Inherited via diagnostics_consumer
    void consume_diagnostics(diagnostic_list diagnostics, fade_message_list fade_messages) override
    {
        diags = diagnostics;
        fms = fade_messages;
    }

    diagnostic_list diags;
    fade_message_list fms;
};
