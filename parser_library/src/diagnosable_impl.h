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

#ifndef HLASMPLUGIN_PARSERLIBRARY_DIAGNOSABLE_IMPL_H
#define HLASMPLUGIN_PARSERLIBRARY_DIAGNOSABLE_IMPL_H

#include "diagnosable.h"

namespace hlasm_plugin::parser_library {

// Abstract class that implements add_diagnostic and diags
// collect_diags is still abstract.
template<typename T>
class collectable_impl : public virtual collectable<T>
{
public:
    virtual typename collectable<T>::diagnostic_container& diags() const override { return container; }

protected:
    // Collects diagnostics from one collectable: calls its collect_diags
    // and then moves or copies the diagnostics depending on is_once_only
    virtual void collect_diags_from_child(const collectable<T>& child) const
    {
        child.collect_diags();
        if (child.is_once_only())
        {
            container.insert(container.end(),
                std::make_move_iterator(child.diags().begin()),
                std::make_move_iterator(child.diags().end()));
            child.diags().clear();
        }
        else
        {
            container.insert(container.end(), child.diags().begin(), child.diags().end());
        }
    }

    virtual void add_diagnostic(T diagnostic) const override { container.push_back(std::move(diagnostic)); }

    virtual bool is_once_only() const override { return true; }

    virtual ~collectable_impl<T>() {};

private:
    mutable typename collectable<T>::diagnostic_container container;
};

using diagnosable_impl = collectable_impl<diagnostic_s>;
using diagnosable_op_impl = collectable_impl<diagnostic_op>;

} // namespace hlasm_plugin::parser_library


#endif