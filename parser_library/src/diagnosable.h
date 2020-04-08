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

#ifndef HLASMPARSER_PARSERLIBRARY_DIAGNOSTICABLE_H
#define HLASMPARSER_PARSERLIBRARY_DIAGNOSTICABLE_H

#include <string>
#include <vector>

#include "diagnostic.h"
#include "protocol.h"

// Interface that allows to collect objects (diagnostics)
// from a tree structure of objects. Each class that implements
// this interface must implement method collect_diags, that
// moves diagnostics from its children to itself.
// In the result, after collect_diags is called in the root,
// all diagnostics from all the objects can be accessed by
// calling diags()

namespace hlasm_plugin::parser_library {

// Interface that allows to collect objects (diagnostics)
// from a tree structure of objects.
template<typename T> class collectable
{
public:
    using diagnostic_container = std::vector<T>;

    virtual void collect_diags() const = 0;
    virtual diagnostic_container& diags() const = 0;
    virtual void add_diagnostic(T diagnostic) const = 0;
    // Specifies whether objects(diagnostics) should be moved
    // when collecting from this object.
    virtual bool is_once_only() const = 0;

    virtual ~collectable() = 0;
};

template<typename T> inline collectable<T>::~collectable() {};

using diagnosable = collectable<diagnostic_s>;

} // namespace hlasm_plugin::parser_library

#endif
