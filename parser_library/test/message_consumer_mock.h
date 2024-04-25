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

#ifndef HLASMPLUGIN_PARSERLIBRARY_MESSAGE_CONSUMER_MOCK_H
#define HLASMPLUGIN_PARSERLIBRARY_MESSAGE_CONSUMER_MOCK_H

#include <string>
#include <utility>
#include <vector>

#include "message_consumer.h"

namespace hlasm_plugin::parser_library {

class message_consumer_mock : public hlasm_plugin::parser_library::message_consumer
{
public:
    void show_message(std::string_view message, message_type type) override { messages.emplace_back(message, type); }
    std::vector<std::pair<std::string, message_type>> messages;
};

} // namespace hlasm_plugin::parser_library
#endif
