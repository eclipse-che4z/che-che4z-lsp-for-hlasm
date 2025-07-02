/*
 * Copyright (c) 2025 Broadcom.
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

#ifndef HLASMPLUGIN_HLASMLANGUAGESERVER_LSP_WATCHERS_H
#define HLASMPLUGIN_HLASMLANGUAGESERVER_LSP_WATCHERS_H

#include <string_view>

#include "nlohmann/json_fwd.hpp"
#include "watcher_registration_provider.h"

namespace hlasm_plugin::language_server::lsp {

nlohmann::json watcher_registeration(parser_library::watcher_registration_id id, std::string_view uri, bool r);
nlohmann::json watcher_unregisteration(parser_library::watcher_registration_id id);
nlohmann::json default_watcher_registration();

} // namespace hlasm_plugin::language_server::lsp

#endif
