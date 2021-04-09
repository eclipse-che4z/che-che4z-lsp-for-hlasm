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

#ifndef HLASMPLUGIN_HLASMLANGUAGESERVER_MESSAGE_ROUTER_H
#define HLASMPLUGIN_HLASMLANGUAGESERVER_MESSAGE_ROUTER_H

#include <functional>
#include <utility>
#include <vector>

#include "json_channel.h"

namespace hlasm_plugin::language_server {
class message_router final : public json_sink
{
public:
    using message_predicate = std::function<bool(const nlohmann::json&)>;

private:
    std::vector<std::pair<message_predicate, json_sink*>> routes;
    json_sink* default_route;

public:
    explicit message_router(json_sink* optional_default_route = nullptr);
    void register_route(message_predicate predicate, json_sink& sink);

    void write(const nlohmann::json&) override;
    void write(nlohmann::json&&) override;
};
} // namespace hlasm_plugin::language_server

#endif // HLASMPLUGIN_HLASMLANGUAGESERVER_MESSAGE_ROUTER_H
