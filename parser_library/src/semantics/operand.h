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

#include <concepts>
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
private:
    template<typename T>
    [[nodiscard]] constexpr T* cast() noexcept
    {
        return type == T::type_id ? static_cast<T*>(this) : nullptr;
    }

    template<typename T>
    [[nodiscard]] constexpr const T* cast() const noexcept
    {
        return type == T::type_id ? static_cast<const T*>(this) : nullptr;
    }

public:
    operand(const operand_type type, const range& operand_range);

    virtual void apply(operand_visitor& visitor) const = 0;

    template<std::same_as<model_operand> T = model_operand>
    [[nodiscard]] constexpr T* access_model() noexcept
    {
        return cast<T>();
    }
    template<std::same_as<ca_operand> T = ca_operand>
    [[nodiscard]] constexpr T* access_ca() noexcept
    {
        return cast<T>();
    }
    template<std::same_as<macro_operand> T = macro_operand>
    [[nodiscard]] constexpr T* access_mac() noexcept
    {
        return cast<T>();
    }
    template<std::same_as<data_def_operand> T = data_def_operand>
    [[nodiscard]] constexpr T* access_data_def() noexcept
    {
        return cast<T>();
    }
    template<std::same_as<machine_operand> T = machine_operand>
    [[nodiscard]] constexpr T* access_mach() noexcept
    {
        return cast<T>();
    }
    template<std::same_as<assembler_operand> T = assembler_operand>
    [[nodiscard]] constexpr T* access_asm() noexcept
    {
        return cast<T>();
    }

    template<std::same_as<model_operand> T = model_operand>
    [[nodiscard]] constexpr const T* access_model() const noexcept
    {
        return cast<T>();
    }
    template<std::same_as<ca_operand> T = ca_operand>
    [[nodiscard]] constexpr const T* access_ca() const noexcept
    {
        return cast<T>();
    }
    template<std::same_as<macro_operand> T = macro_operand>
    [[nodiscard]] constexpr const T* access_mac() const noexcept
    {
        return cast<T>();
    }
    template<std::same_as<data_def_operand> T = data_def_operand>
    [[nodiscard]] constexpr const T* access_data_def() const noexcept
    {
        return cast<T>();
    }
    template<std::same_as<machine_operand> T = machine_operand>
    [[nodiscard]] constexpr const T* access_mach() const noexcept
    {
        return cast<T>();
    }
    template<std::same_as<assembler_operand> T = assembler_operand>
    [[nodiscard]] constexpr const T* access_asm() const noexcept
    {
        return cast<T>();
    }

    const operand_type type;
    const range operand_range;

    virtual ~operand() = default;
};

void transform_reloc_imm_operands(semantics::operand_list& op_list, const processing::op_code& op);

} // namespace hlasm_plugin::parser_library::semantics


#endif
