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

#include "antlr4-runtime.h"

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
struct parser_holder;
class hlasmparser;

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

    void enable_continuation();
    void disable_continuation();
    bool is_self_def();
    bool is_var_def();

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

    bool MACH();
    bool ASM();
    bool DAT();
    bool ALIAS();
    static bool is_previous_attribute_consuming(bool top_level, const antlr4::Token* token);

    void add_diagnostic(diagnostic_severity severity, std::string code, std::string message, range diag_range) const;
    void add_diagnostic(diagnostic_op d) const;

    context::id_index add_id(std::string s);

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
    std::unique_ptr<hlasmparser> parser;

    ~parser_holder();

    static std::unique_ptr<parser_holder> create(
        semantics::source_info_processor* lsp_proc, context::hlasm_context* hl_ctx, diagnostic_op_consumer* d);
};

} // namespace hlasm_plugin::parser_library::parsing

#endif
