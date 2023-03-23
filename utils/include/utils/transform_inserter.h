/*
 * Copyright (c) 2023 Broadcom.
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

#ifndef HLASMPLUGIN_UTILS_TRANSFORM_INSERTER_H
#define HLASMPLUGIN_UTILS_TRANSFORM_INSERTER_H

#include <functional>
#include <iterator>
#include <utility>

namespace hlasm_plugin::utils {

template<typename C, typename Transform>
class transform_inserter
{
    C* container;
    [[no_unique_address]] Transform transform;

public:
    using iterator_category = std::output_iterator_tag;
    using value_type = void;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = void;

    transform_inserter(C& c, Transform t)
        : container(&c)
        , transform(std::move(t))
    {}

    template<typename V>
    transform_inserter& operator=(V&& value)
    {
        container->insert(std::invoke(transform, std::forward<V>(value)));
        return *this;
    }
    transform_inserter& operator*() { return *this; }
    transform_inserter& operator++() { return *this; }
    transform_inserter& operator++(int) { return *this; }
};

} // namespace hlasm_plugin::utils

#endif
