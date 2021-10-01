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

#ifndef HLASMPLUGIN_LANGUAGESERVER_PARSING_METADATA_SINK_H
#define HLASMPLUGIN_LANGUAGESERVER_PARSING_METADATA_SINK_H

#include "nlohmann/json.hpp"
#include "workspace_manager.h"

namespace hlasm_plugin::language_server {

struct parsing_metadata_collector : public parser_library::parsing_metadata_consumer
{
    void consume_parsing_metadata(const parser_library::parsing_metadata& metadata) override { data = metadata; }

    parser_library::parsing_metadata data;
};

} // namespace hlasm_plugin::language_server



#endif