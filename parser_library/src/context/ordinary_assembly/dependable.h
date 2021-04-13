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

#ifndef CONTEXT_DEPENDABLE_H
#define CONTEXT_DEPENDABLE_H

#include "dependency_collector.h"
#include "symbol.h"

namespace hlasm_plugin::parser_library::context {

// interface for obtaining symbol from its name
class dependency_solver
{
public:
    virtual const symbol* get_symbol(id_index name) const = 0;
};

// interface of an object that depends on another objects (addresses or symbols)
class dependable
{
public:
    virtual dependency_collector get_dependencies(dependency_solver& solver) const = 0;

    virtual ~dependable() = default;
};

// interface for obtaining symbol value from the object
class resolvable : public dependable
{
public:
    virtual symbol_value resolve(dependency_solver& solver) const = 0;
};

} // namespace hlasm_plugin::parser_library::context
#endif
