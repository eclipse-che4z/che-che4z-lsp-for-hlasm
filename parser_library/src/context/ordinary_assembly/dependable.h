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

#include <optional>
#include <variant>

#include "dependency_collector.h"

namespace hlasm_plugin::parser_library::expressions {
struct data_definition;
} // namespace hlasm_plugin::parser_library::expressions

namespace hlasm_plugin::parser_library::context {

class symbol;
struct symbol_value;
struct using_evaluate_result;
struct symbol_candidate
{
    bool mentioned;
};

// interface for obtaining symbol from its name
class dependency_solver
{
public:
    virtual const symbol* get_symbol(id_index name) const = 0;
    virtual std::optional<address> get_loctr() const = 0;
    virtual id_index get_literal_id(const std::shared_ptr<const expressions::data_definition>&) = 0;
    virtual bool using_active(id_index label, const section* sect) const = 0;
    virtual using_evaluate_result using_evaluate(
        id_index label, const section* owner, int32_t offset, bool long_offset) const = 0;
    virtual std::variant<const symbol*, symbol_candidate> get_symbol_candidate(id_index name) const = 0;
    virtual std::string get_opcode_attr(id_index symbol) const = 0;

protected:
    ~dependency_solver() = default;
};

// interface of an object that depends on another objects (addresses or symbols)
class dependable
{
public:
    virtual dependency_collector get_dependencies(dependency_solver& solver) const = 0;

protected:
    ~dependable() = default;
};

// interface for obtaining symbol value from the object
class resolvable : public dependable
{
public:
    virtual symbol_value resolve(dependency_solver& solver) const = 0;

protected:
    ~resolvable() = default;
};

} // namespace hlasm_plugin::parser_library::context
#endif
