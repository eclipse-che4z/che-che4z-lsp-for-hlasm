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

#ifndef HLASMPLUGIN_PARSERLIBRARY_PARSER_IMPL_H
#define HLASMPLUGIN_PARSERLIBRARY_PARSER_IMPL_H

#include <type_traits>

#include "Parser.h"
#include "parser_error_listener.h"
#include "semantics/collector.h"
#include "semantics/source_info_processor.h"

namespace hlasm_plugin::parser_library::context {
class hlasm_context;
} // namespace hlasm_plugin::parser_library::context

namespace hlasm_plugin::parser_library::lexing {
class input_source;
class lexer;
class token_stream;
} // namespace hlasm_plugin::parser_library::lexing

namespace hlasm_plugin::parser_library::parsing {

using self_def_t = std::int32_t;

class error_strategy;
class hlasmparser_singleline;
class hlasmparser_multiline;

// class providing methods helpful for parsing and methods modifying parsing process
class parser_impl : public antlr4::Parser
{
public:
    parser_impl(antlr4::TokenStream* input);

    void initialize(context::hlasm_context* hlasm_ctx, diagnostic_op_consumer* diagnoser);

    void reinitialize(context::hlasm_context* hlasm_ctx,
        semantics::range_provider range_prov,
        processing::processing_status proc_stat,
        diagnostic_op_consumer* diagnoser);

    void set_diagnoser(diagnostic_op_consumer* diagnoser)
    {
        diagnoser_ = diagnoser;
        err_listener_.diagnoser = diagnoser;
    }

    semantics::collector& get_collector() { return collector; }

    static bool is_attribute_consuming(char c);
    static bool is_attribute_consuming(const antlr4::Token* token);

    static bool can_attribute_consume(char c);
    static bool can_attribute_consume(const antlr4::Token* token);

protected:
    class literal_controller
    {
        enum class request_t
        {
            none,
            off,
            on,
        } request = request_t::none;
        parser_impl& impl;

    public:
        explicit literal_controller(parser_impl& impl)
            : impl(impl)
        {}
        literal_controller(parser_impl& impl, bool restore)
            : request(restore ? request_t::on : request_t::off)
            , impl(impl)
        {}
        literal_controller(literal_controller&& oth) noexcept
            : request(std::exchange(oth.request, request_t::none))
            , impl(oth.impl)
        {}
        ~literal_controller()
        {
            switch (request)
            {
                case request_t::off:
                    impl.literals_allowed = false;
                    break;
                case request_t::on:
                    impl.literals_allowed = true;
                    break;
                default:
                    break;
            }
        }
    };

    void enable_lookahead_recovery();
    void disable_lookahead_recovery();
    void enable_continuation();
    void disable_continuation();
    bool is_self_def();

    bool allow_ca_string() const { return ca_string_enabled; }
    void enable_ca_string() { ca_string_enabled = true; }
    void disable_ca_string() { ca_string_enabled = false; }

    bool allow_literals() const { return literals_allowed; }
    literal_controller enable_literals()
    {
        if (literals_allowed)
            return literal_controller(*this);

        literals_allowed = true;
        return literal_controller(*this, false);
    }
    literal_controller disable_literals()
    {
        if (!literals_allowed)
            return literal_controller(*this);

        literals_allowed = false;
        return literal_controller(*this, true);
    }

    self_def_t parse_self_def_term(const std::string& option, const std::string& value, range term_range);
    self_def_t parse_self_def_term_in_mach(const std::string& option, const std::string& value, range term_range);
    context::data_attr_kind get_attribute(std::string attr_data);
    context::id_index parse_identifier(std::string value, range id_range);
    size_t get_loctr_len() const;
    bool loctr_len_allowed(const std::string& attr) const;

    void resolve_expression(expressions::ca_expr_ptr& expr, context::SET_t_enum type) const;
    void resolve_expression(std::vector<expressions::ca_expr_ptr>& expr, context::SET_t_enum type) const;
    void resolve_expression(expressions::ca_expr_ptr& expr) const;
    void resolve_concat_chain(const semantics::concat_chain& chain) const;

    lexing::token_stream& input;
    context::hlasm_context* hlasm_ctx = nullptr;
    std::optional<processing::processing_status> proc_status;
    semantics::collector collector;
    semantics::range_provider provider;

    bool ALIAS();
    bool END();
    bool NOT(const antlr4::Token* token) const;

    void add_diagnostic(diagnostic_severity severity, std::string code, std::string message, range diag_range) const;
    void add_diagnostic(diagnostic_op d) const;

    context::id_index add_id(std::string s) const;
    context::id_index add_id(std::string_view s) const;

    void add_label_component(
        const antlr4::Token* token, semantics::concat_chain& chain, std::string& buffer, bool& has_variables) const;
    void add_label_component(
        semantics::vs_ptr s, semantics::concat_chain& chain, std::string& buffer, bool& has_variables) const;

    std::string get_context_text(const antlr4::ParserRuleContext* ctx) const;

private:
    antlr4::misc::IntervalSet getExpectedTokens() override;
    diagnostic_op_consumer* diagnoser_ = nullptr;
    parser_error_listener err_listener_;

    bool ca_string_enabled = true;
    bool literals_allowed = true;
};

// structure containing parser components
struct parser_holder
{
    std::shared_ptr<parsing::error_strategy> error_handler;
    std::unique_ptr<lexing::input_source> input;
    std::unique_ptr<lexing::lexer> lex;
    std::unique_ptr<lexing::token_stream> stream;
    std::unique_ptr<parser_impl> parser;

    virtual ~parser_holder();

    struct op_data
    {
        std::optional<std::string> op_text;
        range op_range;
        size_t op_logical_column;
    };

    virtual op_data lab_instr() const = 0;
    virtual op_data look_lab_instr() const = 0;

    virtual void op_rem_body_noop() const = 0;
    virtual void op_rem_body_ignored() const = 0;
    virtual void op_rem_body_deferred() const = 0;
    virtual void lookahead_operands_and_remarks_asm() const = 0;
    virtual void lookahead_operands_and_remarks_dat() const = 0;

    virtual semantics::op_rem op_rem_body_mac_r() const = 0;
    virtual semantics::operand_list macro_ops() const = 0;
    virtual semantics::op_rem op_rem_body_asm_r() const = 0;
    virtual semantics::op_rem op_rem_body_mach_r() const = 0;
    virtual semantics::op_rem op_rem_body_dat_r() const = 0;

    virtual void op_rem_body_ca_expr() const = 0;
    virtual void op_rem_body_ca_branch() const = 0;
    virtual void op_rem_body_ca_var_def() const = 0;

    virtual void op_rem_body_dat() const = 0;
    virtual void op_rem_body_mach() const = 0;
    virtual void op_rem_body_asm() const = 0;

    virtual semantics::operand_ptr ca_op_expr() const = 0;
    virtual semantics::operand_ptr operand_mach() const = 0;

    struct mac_op_data
    {
        semantics::op_rem operands;
        range op_range;
        size_t op_logical_column;
    };

    virtual mac_op_data op_rem_body_mac() const = 0;

    virtual semantics::literal_si literal_reparse() const = 0;

    void prepare_parser(std::string_view text,
        context::hlasm_context* hlasm_ctx,
        diagnostic_op_consumer* diags,
        semantics::range_provider range_prov,
        range text_range,
        size_t logical_column,
        const processing::processing_status& proc_status,
        bool unlimited_line) const;

    static std::unique_ptr<parser_holder> create(semantics::source_info_processor* lsp_proc,
        context::hlasm_context* hl_ctx,
        diagnostic_op_consumer* d,
        bool multiline);
};

} // namespace hlasm_plugin::parser_library::parsing

#endif
