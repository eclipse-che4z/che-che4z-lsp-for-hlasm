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

#include "context/hlasm_context.h"
#include "diagnosable.h"
#include "lexing/lexer.h"
#include "processing/opencode_provider.h"
#include "processing/statement_fields_parser.h"
#include "processing/statement_providers/statement_provider.h"
#include "semantics/collector.h"
#include "semantics/source_info_processor.h"

namespace hlasm_plugin::parser_library::lexing {
class input_source;
class lexer;
class token_stream;
} // namespace hlasm_plugin::parser_library::lexing

namespace hlasm_plugin::parser_library::parsing {

using self_def_t = std::int32_t;

struct parser_holder;
class hlasmparser;

// class providing methods helpful for parsing and methods modifying parsing process
class parser_impl : public antlr4::Parser,
                    public diagnosable_impl,
                    public processing::opencode_provider,
                    public processing::statement_fields_parser
{
public:
    parser_impl(antlr4::TokenStream* input);

    void initialize(analyzing_context ctx,
        semantics::source_info_processor* src_prc,
        workspaces::parse_lib_provider* lib_provider,
        processing::processing_state_listener* state_listener);

    bool is_last_line() const;
    void rewind_input(context::source_position pos) override;
    context::source_position statement_start() const;
    context::source_position statement_end() const;

    processing::statement_fields_parser::parse_result parse_operand_field(std::string field,
        bool after_substitution,
        semantics::range_provider field_range,
        processing::processing_status status) override;

    context::shared_stmt_ptr get_next(const processing::statement_processor& processor) override;

    void collect_diags() const override;
    std::vector<antlr4::ParserRuleContext*> tree;

protected:
    void enable_continuation();
    void disable_continuation();
    void enable_hidden();
    void disable_hidden();
    bool is_self_def();
    bool is_data_attr();
    bool is_var_def();
    self_def_t parse_self_def_term(const std::string& option, const std::string& value, range term_range);
    context::data_attr_kind get_attribute(std::string attr_data, range data_range);
    context::id_index parse_identifier(std::string value, range id_range);
    void parse_macro_operands(semantics::op_rem& line);

    void resolve_expression(expressions::ca_expr_ptr& expr, context::SET_t_enum type) const;
    void resolve_expression(std::vector<expressions::ca_expr_ptr>& expr, context::SET_t_enum type) const;
    void resolve_expression(expressions::ca_expr_ptr& expr) const;


    bool finished() const override;

    void set_source_indices(const antlr4::Token* start, const antlr4::Token* stop);

    lexing::token_stream& input;
    analyzing_context ctx;
    context::hlasm_context* hlasm_ctx = nullptr;
    semantics::source_info_processor* src_proc = nullptr;
    const processing::statement_processor* processor = nullptr;
    context::shared_stmt_ptr current_statement;
    std::optional<processing::processing_status> proc_status;
    bool finished_flag;
    semantics::collector collector;
    semantics::range_provider provider;

    bool deferred();
    bool no_op();
    bool ignored();
    bool alt_format();
    bool MACH();
    bool ASM();
    bool DAT();
    bool CA();
    bool MAC();
    bool UNKNOWN();

private:
    std::unique_ptr<parser_holder> rest_parser_;
    workspaces::parse_lib_provider* lib_provider_ = nullptr;
    processing::processing_state_listener* state_listener_ = nullptr;

    void initialize(context::hlasm_context* hlasm_ctx,
        semantics::range_provider range_prov,
        processing::processing_status proc_stat);

    semantics::operand_list parse_macro_operands(
        std::string operands, range field_range, std::vector<range> operand_ranges);

    // process methods return true if attribute lookahead needed
    bool process_instruction();
    bool process_statement();

    void process_ordinary();
    void process_lookahead();

    void parse_operands(const std::string& text, range text_range);
    void parse_lookahead_operands(const std::string& text, range text_range);

    antlr4::misc::IntervalSet getExpectedTokens() override;
};

// structure containing parser components
struct parser_holder
{
    std::unique_ptr<lexing::input_source> input;
    std::unique_ptr<lexing::lexer> lex;
    std::unique_ptr<lexing::token_stream> stream;
    std::unique_ptr<hlasmparser> parser;

    parser_holder(const parser_holder&) = delete;
    parser_holder() = default;

    ~parser_holder();
};

} // namespace hlasm_plugin::parser_library::parsing

#endif
