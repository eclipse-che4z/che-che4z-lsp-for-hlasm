/*
 * Copyright (c) 2022 Broadcom.
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


#ifndef HLASMPLUGIN_LANGUAGESERVER_VIRTUAL_FILE_PROVIDER_H
#define HLASMPLUGIN_LANGUAGESERVER_VIRTUAL_FILE_PROVIDER_H

#include "json_channel.h"

#include "message_router.h"

namespace hlasm_plugin::parser_library {
class workspace_manager;
}

namespace hlasm_plugin::language_server {

class virtual_file_provider final : public json_sink
{
    hlasm_plugin::parser_library::workspace_manager* ws_mngr;
    json_sink* out_stream;

public:
    // Inherited via json_sink
    void write(const nlohmann::json& m) override;
    void write(nlohmann::json&& m) override;

    virtual_file_provider(hlasm_plugin::parser_library::workspace_manager& ws_mngr, json_sink& out)
        : ws_mngr(&ws_mngr)
        , out_stream(&out)
    {}

    [[nodiscard]] message_router::message_predicate get_filtering_predicate() const;
};

} // namespace hlasm_plugin::language_server

#endif
