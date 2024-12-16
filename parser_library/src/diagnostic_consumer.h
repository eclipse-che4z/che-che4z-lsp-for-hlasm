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
#include "diagnostic_op.h"

// Interface that allows to consume diagnostics regardless of how are they processed afterwards

namespace hlasm_plugin::parser_library {
struct diagnostic;
struct diagnostic_op;

// Interface that allows to collect objects (diagnostics)
// from a tree structure of objects.
template<typename T>
class diagnostic_consumer_t
{
public:
    virtual void add_diagnostic(T diagnostic) = 0;

protected:
    ~diagnostic_consumer_t() = default;
};

using diagnostic_consumer = diagnostic_consumer_t<diagnostic>;
using diagnostic_op_consumer = diagnostic_consumer_t<diagnostic_op>;

namespace transform_traits {
template<typename R, typename T>
T arg0(std::function<R(T)>);

template<typename T>
using arg0_t = decltype(arg0(std::function(std::declval<T>())));
} // namespace transform_traits

template<typename F, typename T = typename transform_traits::arg0_t<F>>
class diagnostic_consumer_transform final : public diagnostic_consumer_t<T>
{
    F consumer;

public:
    explicit diagnostic_consumer_transform(F f)
        : consumer(std::move(f))
    {}
    void add_diagnostic(T diagnostic) override { consumer(std::move(diagnostic)); };
};

template<typename T>
class drop_diagnostics_t final : public diagnostic_consumer_t<T>
{
public:
    void add_diagnostic(T) override { /* drop the diagnostic */ };
};

inline constinit drop_diagnostics_t<diagnostic_op> drop_diagnostic_op;
inline constinit drop_diagnostics_t<diagnostic> drop_diagnostic;

template<typename T>
class diagnostic_consumer_container final : public diagnostic_consumer_t<T>
{
public:
    std::vector<T> diags;
    void add_diagnostic(T diagnostic) override { diags.push_back(std::move(diagnostic)); };
};

using diagnostic_op_consumer_container = diagnostic_consumer_container<diagnostic_op>;

} // namespace hlasm_plugin::parser_library

#endif
