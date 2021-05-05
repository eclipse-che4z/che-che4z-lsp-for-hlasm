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

#ifndef PROCESSING_OPENCODE_PROVIDER_H
#define PROCESSING_OPENCODE_PROVIDER_H

#include "context/source_snapshot.h"
#include "statement_providers/statement_provider.h"

namespace hlasm_plugin::parser_library::processing {

// interface for hiding parser implementation
class opencode_provider : public statement_provider
{
public:
    // rewinds position in file
    virtual void rewind_input(context::source_position pos) = 0;
    virtual std::string aread() = 0;

    opencode_provider()
        : statement_provider(processing::statement_provider_kind::OPEN)
    {}

    virtual ~opencode_provider() = default;
};

} // namespace hlasm_plugin::parser_library::processing
#endif
