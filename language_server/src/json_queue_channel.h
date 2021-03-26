/*
 * Copyright (c) 2021 Broadcom.
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

#ifndef HLASMPLUGIN_HLASMLANGUAGESERVER_JSON_QUEUE_CHANNEL_H
#define HLASMPLUGIN_HLASMLANGUAGESERVER_JSON_QUEUE_CHANNEL_H

#include "json.hpp"
#include "json_channel.h"

#include "blocking_queue.h"

namespace hlasm_plugin::language_server {
class json_queue_channel final : public json_channel
{
    blocking_queue<nlohmann::json> queue;

public:
    std::optional<nlohmann::json> read() override;

    void write(const nlohmann::json&) override;
    void write(nlohmann::json&&) override;

    void terminate();
};
} // namespace hlasm_plugin::language_server

#endif // HLASMPLUGIN_HLASMLANGUAGESERVER_JSON_QUEUE_CHANNEL_H
