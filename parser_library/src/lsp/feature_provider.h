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

#ifndef LSP_FEATURE_PROVIDER_H
#define LSP_FEATURE_PROVIDER_H

#include "completion_item.h"
#include "document_symbol_item.h"
#include "location.h"
#include "protocol.h"


namespace hlasm_plugin::parser_library::lsp {

using hover_result = std::string;

struct feature_provider
{
    virtual location definition(const std::string& document_uri, position pos) const = 0;
    virtual location_list references(const std::string& document_uri, position pos) const = 0;
    virtual hover_result hover(const std::string& document_uri, position pos) const = 0;
    virtual completion_list_s completion(const std::string& document_uri,
        position pos,
        char trigger_char,
        completion_trigger_kind trigger_kind) const = 0;
    virtual document_symbol_list_s document_symbol(const std::string& document_uri) const = 0;

protected:
    ~feature_provider() = default;
};

} // namespace hlasm_plugin::parser_library::lsp

#endif
