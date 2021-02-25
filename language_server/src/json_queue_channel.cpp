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

#include "json_queue_channel.h"

namespace hlasm_plugin::language_server {

std::optional<nlohmann::json> hlasm_plugin::language_server::json_queue_channel::read() { return queue.pop(); }

void json_queue_channel::write(const nlohmann::json& json) { queue.push(json); }
void json_queue_channel::write(nlohmann::json&& json) { queue.push(std::move(json)); }

void json_queue_channel::terminate() { queue.terminate(); }

} // namespace hlasm_plugin::language_server