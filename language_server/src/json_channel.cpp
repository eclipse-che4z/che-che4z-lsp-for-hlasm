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

#include "json_channel.h"

#include "nlohmann/json.hpp"

namespace hlasm_plugin::language_server {
std::optional<nlohmann::json> json_channel_adapter::read() { return source.read(); }
void json_channel_adapter::write(const nlohmann::json& j) { sink.write(j); }
void json_channel_adapter::write(nlohmann::json&& j) { sink.write(std::move(j)); }
} // namespace hlasm_plugin::language_server
