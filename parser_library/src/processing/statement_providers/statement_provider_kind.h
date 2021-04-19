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

#ifndef PROCESSING_STATEMENT_PROVIDER_KIND_H
#define PROCESSING_STATEMENT_PROVIDER_KIND_H

namespace hlasm_plugin::parser_library::processing {

// kind of statement providers
enum class statement_provider_kind
{
    MACRO = 0,
    COPY = 1,
    OPEN = 2 // MACRO = 0, AINS = 1, COPY = 2, OPEN = 3 -- priority
};

} // namespace hlasm_plugin::parser_library::processing
#endif
