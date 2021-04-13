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

#ifndef PROCESSING_BRANCHING_PROVIDER_H
#define PROCESSING_BRANCHING_PROVIDER_H

#include "context/id_storage.h"
#include "context/ordinary_assembly/dependant.h"
#include "range.h"

namespace hlasm_plugin::parser_library::processing {

// interface for registering and using sequence symbols
class branching_provider
{
public:
    virtual void jump_in_statements(context::id_index target, range symbol_range) = 0;
    virtual void register_sequence_symbol(context::id_index target, range symbol_range) = 0;

    virtual ~branching_provider() = default;
};

} // namespace hlasm_plugin::parser_library::processing
#endif
