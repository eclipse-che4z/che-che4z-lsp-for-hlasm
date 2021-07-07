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

#ifndef HLASMPARSER_PARSERLIBRARY_DIAGNOSTIC_CONSUMER_H
#define HLASMPARSER_PARSERLIBRARY_DIAGNOSTIC_CONSUMER_H

#include <functional>
#include <vector>

#include "diagnostic.h"

// Interface that allows to consume diagnostics regardless of how are they processed afterwards

namespace hlasm_plugin::parser_library {

// Interface that allows to collect objects (diagnostics)
// from a tree structure of objects.
template<typename T>
class diagnostic_consumer
{
    // TODO The reason why this function is const is that all current implementations have mutable containters where
    // the diagnostics are stored and large parts of the project depend on that constness of the function
public:
    virtual void add_diagnostic(T diagnostic) const = 0;

protected:
    ~diagnostic_consumer() = default;
};

using diagnostic_s_consumer = diagnostic_consumer<diagnostic_s>;
using diagnostic_op_consumer = diagnostic_consumer<diagnostic_op>;

namespace transform_traits {
template<class signature>
struct args;

// Unwraps functions with one parameter
template<typename THIS, typename T>
struct args<void (THIS::*)(T) const>
{
    using first_arg_t = typename T;
};

// Unwraps functors with one parameter
template<typename F>
struct callable_args
{
    using first_arg_t = typename args<decltype(&F::operator())>::first_arg_t;
};
} // namespace transform_traits

template<typename F, typename T = typename transform_traits::callable_args<F>::first_arg_t>
class diagnostic_consumer_transform : public diagnostic_consumer<T>
{
    F consumer;
public:
    explicit diagnostic_consumer_transform(F f)
        : consumer(std::move(f))
    {}
    void add_diagnostic(T diagnostic) const override { consumer(std::move(diagnostic)); };
};

template<typename T>
class diagnostic_consumer_container : public diagnostic_consumer<T>
{
public:
    mutable std::vector<T> diags;
    void add_diagnostic(T diagnostic) const override { diags.push_back(std::move(diagnostic)); };
};

using diagnostic_op_consumer_container = diagnostic_consumer_container<diagnostic_op>;

} // namespace hlasm_plugin::parser_library

#endif
