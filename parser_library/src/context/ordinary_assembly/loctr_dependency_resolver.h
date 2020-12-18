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

#ifndef CONTEXT_LOCTR_DEPENDENCY_RESOLVER_H
#define CONTEXT_LOCTR_DEPENDENCY_RESOLVER_H

#include "address.h"

namespace hlasm_plugin::parser_library::context {

class loctr_dependency_resolver
{
public:
    virtual void resolve_unknown_loctr_dependency(space_ptr sp, const context::address& addr, range err_range) = 0;

    virtual ~loctr_dependency_resolver() = default;
};

} // namespace hlasm_plugin::parser_library::context

#endif
