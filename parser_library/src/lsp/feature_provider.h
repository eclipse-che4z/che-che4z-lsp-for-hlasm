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

#include "protocol.h"

namespace hlasm_plugin::parser_library::lsp {

struct feature_provider
{
    virtual position_uri definition(const char* document_uri, const position pos) = 0;
    virtual position_uris references(const char* document_uri, const position pos) = 0;
    virtual string_array hover(const char* document_uri, const position pos) = 0;
    virtual completion_list completion(
        const char* document_uri, const position pos, const char trigger_char, int trigger_kind) = 0;
};

} // namespace hlasm_plugin::parser_library::lsp

#endif