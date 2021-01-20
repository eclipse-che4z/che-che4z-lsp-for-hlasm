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

#ifndef SEMANTICS_OPERAND_IMPLS_H
#define SEMANTICS_OPERAND_IMPLS_H

#include <vector>

#include "checking/data_definition/data_definition_operand.h"
#include "checking/instr_operand.h"
#include "concatenation_term.h"
#include "expressions/data_definition.h"
#include "expressions/mach_expression.h"
#include "operand.h"

// the file contains structures representing operands in the operand field of statement

namespace hlasm_plugin::parser_library::semantics {

// structure for empty operands
struct empty_operand final : operand
{
    empty_operand(const range operand_range);
};



// operand that contains variable symbol thus is 'model operand'
struct model_operand final : operand
{
    model_operand(concat_chain chain, const range operand_range);

    concat_chain chain;
};



// operands that can return value and have dependencies
struct evaluable_operand : operand, diagnosable_op_impl
{
    evaluable_operand(const operand_type type, const range operand_range);

    virtual bool has_dependencies(expressions::mach_evaluate_info info) const = 0;

    virtual bool has_error(expressions::mach_evaluate_info info) const = 0;

    virtual std::vector<const context::resolvable*> get_resolvables() const = 0;

    virtual std::unique_ptr<checking::operand> get_operand_value(expressions::mach_evaluate_info info) const = 0;
};



// operand representing simple expression
struct simple_expr_operand : virtual evaluable_operand
{
    simple_expr_operand(expressions::mach_expr_ptr expression);

    bool has_dependencies(expressions::mach_evaluate_info info) const override;

    bool has_error(expressions::mach_evaluate_info info) const override;

    std::vector<const context::resolvable*> get_resolvables() const override;

    expressions::mach_expr_ptr expression;
};


enum class mach_kind
{
    EXPR,
    ADDR
};
struct expr_machine_operand;
struct address_machine_operand;

// machine instruction operand
struct machine_operand : virtual evaluable_operand
{
    machine_operand(const mach_kind kind);

    expr_machine_operand* access_expr();
    address_machine_operand* access_address();

    using evaluable_operand::get_operand_value;
    virtual std::unique_ptr<checking::operand> get_operand_value(
        expressions::mach_evaluate_info info, checking::machine_operand_type type_hint) const = 0;

    const mach_kind kind;
};



// machine expression operand
struct expr_machine_operand final : machine_operand, simple_expr_operand
{
    expr_machine_operand(expressions::mach_expr_ptr expression, const range operand_range);

    std::unique_ptr<checking::operand> get_operand_value(expressions::mach_evaluate_info info) const override;
    std::unique_ptr<checking::operand> get_operand_value(
        expressions::mach_evaluate_info info, checking::machine_operand_type type_hint) const override;

    bool has_dependencies(expressions::mach_evaluate_info info) const override;
    bool has_error(expressions::mach_evaluate_info info) const override;
    std::vector<const context::resolvable*> get_resolvables() const override;

    void collect_diags() const override;
};


// machine address operand
struct address_machine_operand final : machine_operand
{
    address_machine_operand(expressions::mach_expr_ptr displacement,
        expressions::mach_expr_ptr first_par,
        expressions::mach_expr_ptr second_par,
        const range operand_range,
        checking::operand_state state);

    expressions::mach_expr_ptr displacement;
    expressions::mach_expr_ptr first_par;
    expressions::mach_expr_ptr second_par;
    checking::operand_state state;

    bool has_dependencies(expressions::mach_evaluate_info info) const override;

    bool has_error(expressions::mach_evaluate_info info) const override;

    std::vector<const context::resolvable*> get_resolvables() const override;

    std::unique_ptr<checking::operand> get_operand_value(expressions::mach_evaluate_info info) const override;
    std::unique_ptr<checking::operand> get_operand_value(
        expressions::mach_evaluate_info info, checking::machine_operand_type type_hint) const override;

    void collect_diags() const override;
};


enum class asm_kind
{
    EXPR,
    BASE_END,
    COMPLEX,
    STRING
};
struct expr_assembler_operand;
struct using_instr_assembler_operand;
struct complex_assembler_operand;
struct string_assembler_operand;

// assembler instruction operand
struct assembler_operand : virtual evaluable_operand
{
    assembler_operand(const asm_kind kind);

    expr_assembler_operand* access_expr();
    using_instr_assembler_operand* access_base_end();
    complex_assembler_operand* access_complex();
    string_assembler_operand* access_string();

    const asm_kind kind;
};


// assembler expression operand
struct expr_assembler_operand final : assembler_operand, simple_expr_operand
{
private:
    std::string value_;

public:
    expr_assembler_operand(expressions::mach_expr_ptr expression, std::string string_value, const range operand_range);

    std::unique_ptr<checking::operand> get_operand_value(expressions::mach_evaluate_info info) const override;

    std::unique_ptr<checking::operand> get_operand_value(
        expressions::mach_evaluate_info info, bool can_have_ordsym) const;

    bool has_dependencies(expressions::mach_evaluate_info info) const override;
    bool has_error(expressions::mach_evaluate_info info) const override;
    std::vector<const context::resolvable*> get_resolvables() const override;

    void collect_diags() const override;

private:
    std::unique_ptr<checking::operand> get_operand_value_inner(
        expressions::mach_evaluate_info info, bool can_have_ordsym) const;
};



// USING instruction operand
struct using_instr_assembler_operand final : assembler_operand
{
    using_instr_assembler_operand(
        expressions::mach_expr_ptr base, expressions::mach_expr_ptr end, const range operand_range);

    bool has_dependencies(expressions::mach_evaluate_info info) const override;

    bool has_error(expressions::mach_evaluate_info info) const override;

    std::vector<const context::resolvable*> get_resolvables() const override;

    std::unique_ptr<checking::operand> get_operand_value(expressions::mach_evaluate_info info) const override;

    expressions::mach_expr_ptr base;
    expressions::mach_expr_ptr end;

    void collect_diags() const override;
};



// complex assembler operand (i.e. 'OVERRIDE(A,B,C)')
struct complex_assembler_operand final : assembler_operand
{
    struct component_value_t
    {
        component_value_t()
            : op_range(range())
        {}
        component_value_t(range op_range)
            : op_range(op_range)
        {}

        virtual std::unique_ptr<checking::asm_operand> create_operand() const = 0;
        virtual ~component_value_t() = default;

        range op_range;
    };

    struct int_value_t final : component_value_t
    {
        int_value_t(int value, range range)
            : component_value_t(range)
            , value(value)
        {}
        std::unique_ptr<checking::asm_operand> create_operand() const override
        {
            return std::make_unique<checking::one_operand>(value, op_range);
        }
        int value;
    };
    struct string_value_t final : component_value_t
    {
        // string_value_t(std::string value) : value(std::move(value)) {}
        string_value_t(std::string value, range range)
            : component_value_t(range)
            , value(std::move(value))
        {}
        std::unique_ptr<checking::asm_operand> create_operand() const override
        {
            return std::make_unique<checking::one_operand>(value, op_range);
        }
        std::string value;
    };
    struct composite_value_t final : component_value_t
    {
        composite_value_t(std::string identifier, std::vector<std::unique_ptr<component_value_t>> values, range range)
            : component_value_t(range)
            , identifier(std::move(identifier))
            , values(std::move(values))
        {}
        std::unique_ptr<checking::asm_operand> create_operand() const override
        {
            std::vector<std::unique_ptr<checking::asm_operand>> ret;
            for (auto& val : values)
                ret.push_back(val->create_operand());
            return std::make_unique<checking::complex_operand>(identifier, std::move(ret));
        }

        std::string identifier;
        std::vector<std::unique_ptr<component_value_t>> values;
    };

    complex_assembler_operand(
        std::string identifier, std::vector<std::unique_ptr<component_value_t>> values, const range operand_range);

    bool has_dependencies(expressions::mach_evaluate_info info) const override;

    bool has_error(expressions::mach_evaluate_info info) const override;

    std::vector<const context::resolvable*> get_resolvables() const override;

    std::unique_ptr<checking::operand> get_operand_value(expressions::mach_evaluate_info info) const override;

    composite_value_t value;

    void collect_diags() const override;
};



// assembler string operand
struct string_assembler_operand : assembler_operand
{
    string_assembler_operand(std::string value, const range operand_range);

    bool has_dependencies(expressions::mach_evaluate_info info) const override;

    bool has_error(expressions::mach_evaluate_info info) const override;

    std::vector<const context::resolvable*> get_resolvables() const override;

    std::unique_ptr<checking::operand> get_operand_value(expressions::mach_evaluate_info info) const override;

    std::string value;

    void collect_diags() const override;
};

// data definition operand
struct data_def_operand final : evaluable_operand
{
    data_def_operand(expressions::data_definition data_def, const range operand_range);

    std::shared_ptr<expressions::data_definition> value;

    context::dependency_collector get_length_dependencies(expressions::mach_evaluate_info info) const;

    context::dependency_collector get_dependencies(expressions::mach_evaluate_info info) const;

    bool has_dependencies(expressions::mach_evaluate_info info) const override;

    bool has_error(expressions::mach_evaluate_info info) const override;

    std::vector<const context::resolvable*> get_resolvables() const override;

    std::unique_ptr<checking::operand> get_operand_value(expressions::mach_evaluate_info info) const override;

    void collect_diags() const override;
};


enum class ca_kind
{
    VAR,
    EXPR,
    SEQ,
    BRANCH
};
struct var_ca_operand;
struct expr_ca_operand;
struct seq_ca_operand;
struct branch_ca_operand;

// coditional assembly instruction operand
struct ca_operand : operand
{
    ca_operand(const ca_kind kind, const range operand_range);

    var_ca_operand* access_var();
    const var_ca_operand* access_var() const;
    expr_ca_operand* access_expr();
    const expr_ca_operand* access_expr() const;
    seq_ca_operand* access_seq();
    const seq_ca_operand* access_seq() const;
    branch_ca_operand* access_branch();
    const branch_ca_operand* access_branch() const;

    virtual std::set<context::id_index> get_undefined_attributed_symbols(
        const expressions::evaluation_context& eval_ctx) = 0;

    const ca_kind kind;
};

// CA variable symbol operand
struct var_ca_operand final : ca_operand
{
    var_ca_operand(vs_ptr variable_symbol, const range operand_range);

    std::set<context::id_index> get_undefined_attributed_symbols(
        const expressions::evaluation_context& eval_ctx) override;

    vs_ptr variable_symbol;
};

// CA expression operand
struct expr_ca_operand final : ca_operand
{
    expr_ca_operand(expressions::ca_expr_ptr expression, const range operand_range);


    std::set<context::id_index> get_undefined_attributed_symbols(
        const expressions::evaluation_context& eval_ctx) override;

    expressions::ca_expr_ptr expression;
};

// CA sequence symbol operand
struct seq_ca_operand final : ca_operand
{
    seq_ca_operand(seq_sym sequence_symbol, const range operand_range);


    std::set<context::id_index> get_undefined_attributed_symbols(
        const expressions::evaluation_context& eval_ctx) override;

    seq_sym sequence_symbol;
};

// CA branching operand (i.e. (5).here)
struct branch_ca_operand final : ca_operand
{
    branch_ca_operand(seq_sym sequence_symbol, expressions::ca_expr_ptr expression, const range operand_range);


    std::set<context::id_index> get_undefined_attributed_symbols(
        const expressions::evaluation_context& eval_ctx) override;

    seq_sym sequence_symbol;
    expressions::ca_expr_ptr expression;
};


enum class mac_kind
{
    CHAIN,
    STRING
};

struct macro_operand_chain;
struct macro_operand_string;

struct macro_operand : operand
{
    const mac_kind kind;

    macro_operand_chain* access_chain();
    macro_operand_string* access_string();

protected:
    macro_operand(mac_kind kind, range operand_range);
};

// macro instruction operand
struct macro_operand_chain final : macro_operand
{
    macro_operand_chain(concat_chain chain, const range operand_range);

    concat_chain chain;
};

// macro instruction operand
struct macro_operand_string final : macro_operand
{
    macro_operand_string(std::string value, const range operand_range);

    std::string value;
};

} // namespace hlasm_plugin::parser_library::semantics

#endif
