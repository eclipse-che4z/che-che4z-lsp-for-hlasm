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

#ifndef SEMANTICS_STATEMENTFIELDS_H
#define SEMANTICS_STATEMENTFIELDS_H

#include <string>
#include <variant>

#include "context/id_index.h"
#include "expressions/data_definition.h"
#include "lexing/string_with_newlines.h"
#include "operand.h"
#include "variable_symbol.h"

// this file contains structures representing each statement field

namespace hlasm_plugin::parser_library::semantics {

enum class label_si_type
{
    ORD,
    SEQ,
    VAR,
    MAC,
    CONC,
    EMPTY
};

struct ord_symbol_string
{
    context::id_index symbol;
    std::string mixed_case;
};

using label_si_value_t = std::variant<std::string, ord_symbol_string, concat_chain, seq_sym, vs_ptr>;

// struct holding semantic information (si) about label field
struct label_si
{
    struct mac_flag
    {};
    label_si(range field_range, ord_symbol_string value)
        : type(label_si_type::ORD)
        , field_range(std::move(field_range))
        , value(std::move(value))
    {}

    label_si(range field_range, std::string value, mac_flag)
        : type(label_si_type::MAC)
        , field_range(std::move(field_range))
        , value(std::move(value))
    {}

    label_si(range field_range, concat_chain value)
        : type(label_si_type::CONC)
        , field_range(std::move(field_range))
        , value(std::move(value))
    {}

    label_si(range field_range, seq_sym value)
        : type(label_si_type::SEQ)
        , field_range(std::move(field_range))
        , value(std::move(value))
    {}

    label_si(range field_range, vs_ptr value)
        : type(label_si_type::VAR)
        , field_range(std::move(field_range))
        , value(std::move(value))
    {}

    label_si(range field_range)
        : type(label_si_type::EMPTY)
        , field_range(std::move(field_range))
    {}

    label_si_type type;
    range field_range;

    label_si_value_t value;

    void resolve(diagnostic_op_consumer& diags);
};

enum class instruction_si_type
{
    ORD,
    CONC,
    EMPTY
};

using instruction_si_value_t = std::variant<context::id_index, concat_chain>;

// struct holding semantic information (si) about instruction field
struct instruction_si
{
    instruction_si(range field_range, context::id_index value)
        : type(instruction_si_type::ORD)
        , field_range(std::move(field_range))
        , value(std::move(value))
    {}

    instruction_si(range field_range, concat_chain value)
        : type(instruction_si_type::CONC)
        , field_range(std::move(field_range))
        , value(std::move(value))
    {}

    instruction_si(range field_range)
        : type(instruction_si_type::EMPTY)
        , field_range(std::move(field_range))
    {}

    instruction_si_type type;
    range field_range;

    instruction_si_value_t value;

    void resolve(diagnostic_op_consumer& diags);
};

// struct holding semantic information (si) about operand field
struct operands_si
{
    operands_si(range field_range, operand_list operands)
        : field_range(std::move(field_range))
        , value(std::move(operands))
    {}

    range field_range;

    operand_list value;
};

// struct holding semantic information (si) about deferred operand field
struct deferred_operands_si
{
    deferred_operands_si(
        range field_range, size_t logical_column, lexing::u8string_with_newlines field, std::vector<vs_ptr> vars)
        : field_range(std::move(field_range))
        , logical_column(logical_column)
        , value(std::move(field))
        , vars(std::move(vars))
    {}

    range field_range;
    size_t logical_column;

    lexing::u8string_with_newlines value;
    std::vector<vs_ptr> vars;
};

// struct holding semantic information (si) about remark field
struct remarks_si
{
    explicit remarks_si(std::vector<range> remarks)
        : value(std::move(remarks))
    {}

    std::vector<range> value;
};

class literal_si_data
{
    std::string m_text;
    expressions::data_definition m_dd;
    range m_range;
    bool m_referenced_by_reladdr = false;

public:
    literal_si_data(std::string text, expressions::data_definition dd, range r)
        : m_text(std::move(text))
        , m_dd(std::move(dd))
        , m_range(r)
    {}

    void set_referenced_by_reladdr() { m_referenced_by_reladdr = true; }
    bool get_referenced_by_reladdr() const { return m_referenced_by_reladdr; }

    const std::string& get_text() const { return m_text; }
    const expressions::data_definition& get_dd() const { return m_dd; }

    const range& get_range() const { return m_range; }

    bool is_similar(const literal_si_data&) const;
};

using literal_si = std::shared_ptr<literal_si_data>;

} // namespace hlasm_plugin::parser_library::semantics
#endif
