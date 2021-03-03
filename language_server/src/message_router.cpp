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

#include "message_router.h"

hlasm_plugin::language_server::message_router::message_router(json_sink* optional_default_route)
    : default_route(optional_default_route)
{}

void hlasm_plugin::language_server::message_router::register_route(message_predicate predicate, json_sink& sink)
{
    routes.emplace_back(std::move(predicate), &sink);
}

void hlasm_plugin::language_server::message_router::write(const nlohmann::json& msg)
{
    for (const auto& [filter, target] : routes)
    {
        if (filter(msg))
        {
            target->write(msg);
            return;
        }
    }
    if (default_route)
        default_route->write(msg);
}

void hlasm_plugin::language_server::message_router::write(nlohmann::json&& msg)
{
    for (const auto& [filter, target] : routes)
    {
        if (filter(msg))
        {
            target->write(std::move(msg));
            return;
        }
    }
    if (default_route)
        default_route->write(std::move(msg));
}
