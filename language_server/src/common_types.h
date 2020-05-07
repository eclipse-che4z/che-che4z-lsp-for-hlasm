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

#include "json.hpp"
// Types that are used throughout the language server component
namespace hlasm_plugin::language_server {

using json = nlohmann::json;

using method = std::function<void(const json& id, const json& params)>;

using send_message_callback = std::function<void(const json&)>;
} // namespace hlasm_plugin::language_server
