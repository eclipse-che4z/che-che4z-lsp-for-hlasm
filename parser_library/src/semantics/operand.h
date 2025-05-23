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

#ifndef SEMANTICS_OPERAND_H
#define SEMANTICS_OPERAND_H

#include <memory>
#include <vector>

#include "context/id_index.h"
#include "range.h"

// the file contains structures representing operands in the operand field of statement

namespace hlasm_plugin::parser_library::processing {
struct op_code;
}

namespace hlasm_plugin::parser_library::semantics {

enum class operand_type
{
    MACH,
    MAC,
    ASM,
    CA,
    DAT,
    MODEL,
    EMPTY
};

struct model_operand;
struct ca_operand;
struct macro_operand;
struct machine_operand;
struct assembler_operand;
struct data_def_operand;

struct operand;
using operand_ptr = std::unique_ptr<operand>;
using operand_list = std::vector<operand_ptr>;
using remark_list = std::vector<range>;

// operand and remark holding structure
struct op_rem
{
    std::vector<operand_ptr> operands;
    std::vector<range> remarks;
    range line_range;
};

// sequence symbol structure
struct seq_sym
{
    context::id_index name;
    range symbol_range;
};

class operand_visitor;

// struct representing operand of instruction
struct operand
{
    operand(const operand_type type, const range operand_range);

    virtual void apply(operand_visitor& visitor) const = 0;

    model_operand* access_model();
    ca_operand* access_ca();
    macro_operand* access_mac();
    data_def_operand* access_data_def();
    machine_operand* access_mach();
    assembler_operand* access_asm();

    const model_operand* access_model() const;
    const ca_operand* access_ca() const;
    const macro_operand* access_mac() const;
    const data_def_operand* access_data_def() const;
    const machine_operand* access_mach() const;
    const assembler_operand* access_asm() const;

    const operand_type type;
    const range operand_range;

    virtual ~operand() = default;
};

void transform_reloc_imm_operands(semantics::operand_list& op_list, const processing::op_code& op);

} // namespace hlasm_plugin::parser_library::semantics


#endif
