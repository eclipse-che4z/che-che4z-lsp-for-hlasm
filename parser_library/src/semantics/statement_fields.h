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

#include "context/id_storage.h"
#include "operand.h"
#include "variable_symbol.h"

// this file contains structures representing each statement field

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

enum class label_si_type
{
    ORD,
    SEQ,
    VAR,
    MAC,
    CONC,
    EMPTY
};

using label_si_value_t = std::variant<std::string, concat_chain, seq_sym, vs_ptr>;

// struct holding semantic information (si) about label field
struct label_si
{
    struct mac_flag
    {};
    label_si(range field_range, std::string value)
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

    const label_si_type type;
    const range field_range;

    label_si_value_t value;
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
        , value(context::id_storage::empty_id)
    {}

    const instruction_si_type type;
    const range field_range;

    instruction_si_value_t value;
};

// struct holding semantic information (si) about operand field
struct operands_si
{
    operands_si(range field_range, operand_list operands)
        : field_range(std::move(field_range))
        , value(std::move(operands))
    {}

    const range field_range;

    operand_list value;
};

// struct holding semantic information (si) about deferred operand field
struct deferred_operands_si
{
    deferred_operands_si(range field_range, std::string field, std::vector<vs_ptr> vars)
        : field_range(std::move(field_range))
        , value(std::move(field))
        , vars(std::move(vars))
    {}

    const range field_range;

    std::string value;
    std::vector<vs_ptr> vars;
};

// struct holding semantic information (si) about remark field
struct remarks_si
{
    remarks_si(range field_range, std::vector<range> remarks)
        : field_range(std::move(field_range))
        , value(std::move(remarks))
    {}

    const range field_range;

    std::vector<range> value;
};



} // namespace semantics
} // namespace parser_library
} // namespace hlasm_plugin
#endif
