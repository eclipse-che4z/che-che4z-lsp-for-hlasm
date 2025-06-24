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

#include "parser_impl.h"

#include <algorithm>
#include <charconv>
#include <concepts>
#include <cstdint>
#include <utility>

#include "context/hlasm_context.h"
#include "context/literal_pool.h"
#include "context/well_known.h"
#include "expressions/conditional_assembly/ca_expr_policy.h"
#include "expressions/conditional_assembly/ca_expr_visitor.h"
#include "expressions/conditional_assembly/ca_expression.h"
#include "expressions/conditional_assembly/ca_operator_binary.h"
#include "expressions/conditional_assembly/ca_operator_unary.h"
#include "expressions/conditional_assembly/terms/ca_constant.h"
#include "expressions/conditional_assembly/terms/ca_expr_list.h"
#include "expressions/conditional_assembly/terms/ca_function.h"
#include "expressions/conditional_assembly/terms/ca_string.h"
#include "expressions/conditional_assembly/terms/ca_symbol.h"
#include "expressions/conditional_assembly/terms/ca_symbol_attribute.h"
#include "expressions/conditional_assembly/terms/ca_var_sym.h"
#include "expressions/mach_expr_term.h"
#include "expressions/mach_expr_visitor.h"
#include "expressions/mach_expression.h"
#include "expressions/nominal_value.h"
#include "lexing/string_with_newlines.h"
#include "processing/op_code.h"
#include "semantics/operand.h"
#include "semantics/operand_impls.h"
#include "utils/scope_exit.h"
#include "utils/string_operations.h"
#include "utils/truth_table.h"
#include "utils/unicode_text.h"

namespace hlasm_plugin::parser_library::parsing {

parser_holder::parser_holder(context::hlasm_context& hl_ctx, diagnostic_op_consumer* d)
    : hlasm_ctx(&hl_ctx)
    , diagnostic_collector(d)
{}

parser_holder::~parser_holder() = default;

void parser_holder::prepare_parser(lexing::u8string_view_with_newlines text,
    context::hlasm_context& hl_ctx,
    diagnostic_op_consumer* diags,
    semantics::range_provider rp,
    range text_range,
    size_t logical_column,
    const processing::processing_status& ps)
{
    hlasm_ctx = &hl_ctx;
    diagnostic_collector = diags;
    range_prov = std::move(rp);
    proc_status = ps;

    reset(text, text_range.start, logical_column);

    collector.prepare_for_next_statement();
}

constexpr auto EOF_SYMBOL = (char8_t)-1;
template<char8_t... chars>
requires((chars != EOF_SYMBOL) && ...) struct group_t
{
    [[nodiscard]] static constexpr bool matches(char8_t ch) noexcept { return ((ch == chars) || ...); }
};
template<char8_t... chars>
constexpr group_t<chars...> group = {};

template<std::array<char8_t, 256> s>
constexpr auto group_from_string()
{
    constexpr auto n = std::ranges::find(s, u8'\0') - s.begin();
    return []<size_t... i>(std::index_sequence<i...>) {
        return group_t<s[i]...>(); //
    }(std::make_index_sequence<n>());
}

constexpr auto selfdef = group_from_string<{ u8"BXCGbxcg" }>();
constexpr auto mach_attrs = group_from_string<{ u8"OSILTosilt" }>();
constexpr auto all_attrs = group_from_string<{ u8"NKDOSILTnkdosilt" }>();
constexpr auto attr_argument = group_from_string<{ u8"$_#@abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ&=*" }>();

constexpr auto ord_first = utils::create_truth_table(u8"$_#@abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
constexpr auto ord = utils::create_truth_table(u8"$_#@abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
constexpr auto numbers = utils::create_truth_table(u8"0123456789");

[[nodiscard]] constexpr bool char_is_ord_first(char8_t c) noexcept { return c < ord_first.size() && ord_first[c]; }
[[nodiscard]] constexpr bool char_is_ord(char8_t c) noexcept { return c < ord.size() && ord[c]; }
[[nodiscard]] constexpr bool char_is_num(char8_t c) noexcept { return c < numbers.size() && numbers[c]; }

namespace {
std::pair<char_substitution, size_t> append_utf8_with_newlines(
    std::vector<char8_t>& t, std::vector<size_t>& nl, std::vector<size_t>& ll, std::string_view s)
{
    char_substitution subs {};

    size_t utf16_length = 0;
    while (!s.empty())
    {
        char8_t c = s.front();
        if (c < 0x80)
        {
            t.push_back(c);
            s.remove_prefix(1);
            ++utf16_length;
            continue;
        }
        else if (c == lexing::u8string_view_with_newlines::EOL)
        {
            nl.push_back(t.size());
            ll.push_back(std::exchange(utf16_length, 0));
            s.remove_prefix(1);
            continue;
        }
        const auto cs = utils::utf8_prefix_sizes[c];
        if (cs.utf8 && cs.utf8 <= s.size())
        {
            t.push_back(c);
            char32_t v = c & 0b0111'1111u >> cs.utf8;
            for (int i = 1; i < cs.utf8; ++i)
            {
                const char8_t n = s[i] & 0b0011'1111u;
                t.push_back(0x80u | n);
                v = v << 6 | n;
            }

            if (v == utils::substitute_character)
                subs.client = true;

            s.remove_prefix(cs.utf8);
            utf16_length += cs.utf16;
        }
        else
        {
            static constexpr std::u8string_view substitute_character = u8"\U00000FFD";
            subs.server = true;
            t.insert(t.end(), substitute_character.begin(), substitute_character.end());
            s.remove_prefix(1);
            ++utf16_length;
        }
    }

    return { subs, utf16_length };
}
} // namespace

void parser_holder::reset(position file_offset, size_t logical_column, bool process)
{
    process_allowed = process;

    input.push_back(EOF_SYMBOL);
    newlines.push_back((size_t)-1);
    input_state.next = input.data();
    input_state.nl = newlines.data();
    input_state.line = file_offset.line;
    input_state.char_position_in_line = logical_column;
    input_state.char_position_in_line_utf16 = file_offset.column;
    input_state.last = &input.back();

    for (auto bump_line = file_offset.column; auto& ll : line_limits)
    {
        ll += bump_line;
        bump_line = cont;
    }
}

char_substitution parser_holder::reset(
    lexing::u8string_view_with_newlines str, position file_offset, size_t logical_column, bool process)
{
    input.clear();
    newlines.clear();
    line_limits.clear();
    auto [subs, _] = append_utf8_with_newlines(input, newlines, line_limits, str.text);

    reset(file_offset, logical_column, process);

    return subs;
}

char_substitution parser_holder::reset(
    const lexing::logical_line<utils::utf8_iterator<std::string_view::iterator, utils::utf8_utf16_counter>>& l,
    position file_offset,
    size_t logical_column,
    bool process)
{
    char_substitution subs {};

    input.clear();
    newlines.clear();
    line_limits.clear();

    for (size_t i = 0; i < l.segments.size(); ++i)
    {
        const auto& s = l.segments[i];

        auto [subs_update, utf16_rem] = append_utf8_with_newlines(
            input, newlines, line_limits, std::string_view(s.code_begin().base(), s.code_end().base()));

        subs |= subs_update;

        if (i + 1 < l.segments.size())
        {
            newlines.push_back(input.size());
            line_limits.push_back(utf16_rem);
        }
    }

    reset(file_offset, logical_column, process);

    return subs;
}

namespace {
struct parser_position
{
    position pos;

    auto operator<=>(const parser_position&) const noexcept = default;
};

struct parser_range
{
    range r;
};

struct
{
} constexpr failure = {};

template<typename T = void>
struct [[nodiscard]] result_t
{
    bool error = false;
    T value = T {};

    template<typename... Args>
    constexpr explicit(false) result_t(Args&&... args)
        requires(sizeof...(Args) > 0 && std::constructible_from<T, Args...>)
        : value(std::forward<Args>(args)...)
    {}
    constexpr explicit(false) result_t(decltype(failure))
        : error(true)
    {}
};

template<>
struct [[nodiscard]] result_t<void>
{
    bool error = false;

    constexpr result_t() = default;
    constexpr explicit(false) result_t(decltype(failure))
        : error(true)
    {}
};
} // namespace

struct parser2
{
    class [[nodiscard]] literal_controller;
    class concat_chain_builder;
    struct maybe_expr_list
    {
        std::variant<std::vector<expressions::ca_expr_ptr>, expressions::ca_expr_ptr> value;
        bool leading_trailing_spaces;
    };

    ///////////////////////////////////////////////////////////////////

    parser_holder* holder;

    parser_holder::input_state_t input;
    const char8_t* data;

    std::vector<range> remarks;

    bool ca_string_enabled = true;
    bool literals_allowed = true;

    ///////////////////////////////////////////////////////////////////

    [[nodiscard]] constexpr bool is_ord_first() const noexcept { return char_is_ord_first(*input.next); }
    [[nodiscard]] constexpr bool is_ord() const noexcept { return char_is_ord(*input.next); }
    [[nodiscard]] constexpr bool is_num() const noexcept { return char_is_num(*input.next); }

    [[nodiscard]] constexpr bool eof() const noexcept { return *input.next == EOF_SYMBOL; }

    template<char8_t... chars>
    [[nodiscard]] constexpr bool except() const noexcept requires((chars != EOF_SYMBOL) && ...)
    {
        const auto ch = *input.next;
        return ((ch != EOF_SYMBOL) && ... && (ch != chars));
    }

    template<auto... groups>
    [[nodiscard]] constexpr bool follows() const noexcept requires(((&decltype(groups)::matches, true) && ...))
    {
        return [this]<size_t... idx>(std::index_sequence<idx...>) {
            return (decltype(groups)::matches(input.next[idx]) && ...); //
        }(std::make_index_sequence<sizeof...(groups)>());
    }

    template<char8_t... chars>
    [[nodiscard]] constexpr bool follows() const noexcept requires((chars != EOF_SYMBOL) && ...)
    {
        const auto ch = *input.next;
        return ((ch == chars) || ...);
    }

    template<char8_t... chars>
    [[nodiscard]] constexpr bool must_follow() requires((chars != EOF_SYMBOL) && ...)
    {
        if (follows<chars...>())
            return true;

        syntax_error_or_eof();

        return false;
    }

    template<char8_t... chars>
    [[nodiscard]] constexpr bool must_follow(diagnostic_op (&d)(const range&)) requires((chars != EOF_SYMBOL) && ...)
    {
        if (follows<chars...>())
            return true;

        add_diagnostic(d);
        return false;
    }

    template<char8_t... chars>
    [[nodiscard]] constexpr bool match(diagnostic_op (&d)(const range&)) requires((chars != EOF_SYMBOL) && ...)
    {
        if (!follows<chars...>())
        {
            add_diagnostic(d);
            return false;
        }
        consume();
        return true;
    }

    template<char8_t... chars>
    [[nodiscard]] constexpr bool match(hl_scopes s, diagnostic_op (&d)(const range&))
        requires((chars != EOF_SYMBOL) && ...)
    {
        if (!follows<chars...>())
        {
            add_diagnostic(d);
            return false;
        }
        consume(s);
        return true;
    }

    template<char8_t... chars>
    [[nodiscard]] constexpr bool match() requires((chars != EOF_SYMBOL) && ...)
    {
        if (must_follow<chars...>())
        {
            consume();
            return true;
        }
        syntax_error_or_eof();
        return false;
    }

    template<char8_t... chars>
    [[nodiscard]] constexpr bool match(hl_scopes s) requires((chars != EOF_SYMBOL) && ...)
    {
        if (must_follow<chars...>())
        {
            consume(s);
            return true;
        }
        return false;
    }

    template<char8_t... chars>
    [[nodiscard]] constexpr bool try_consume() requires((chars != EOF_SYMBOL) && ...)
    {
        if (follows<chars...>())
        {
            consume();
            return true;
        }
        return false;
    }

    template<char8_t... chars>
    [[nodiscard]] constexpr bool try_consume(hl_scopes s) requires((chars != EOF_SYMBOL) && ...)
    {
        if (follows<chars...>())
        {
            consume(s);
            return true;
        }
        return false;
    }

    [[nodiscard]] bool allow_ca_string() const noexcept { return ca_string_enabled; }
    void enable_ca_string() noexcept { ca_string_enabled = true; }
    void disable_ca_string() noexcept { ca_string_enabled = false; }

    bool are_literals_allowed() const noexcept { return literals_allowed; }
    literal_controller enable_literals() noexcept;
    literal_controller disable_literals() noexcept;

    [[nodiscard]] constexpr bool before_nl() const noexcept;

    constexpr void adjust_lines() noexcept;

    void consume() noexcept;
    void consume(hl_scopes s) noexcept;
    void consume_into(std::string& s);
    void consume_into(std::string& s, hl_scopes scope);

    void consume_spaces() noexcept
    {
        while (follows<u8' '>())
            consume();
    }

    [[nodiscard]] parser_position cur_pos() const noexcept;
    [[nodiscard]] parser_position cur_pos_adjusted() noexcept;
    [[nodiscard]] range cur_pos_range() const noexcept;
    [[nodiscard]] range range_from(parser_position start) const noexcept;
    [[nodiscard]] range empty_range(parser_position start) const noexcept;

    void consume_rest();

    [[nodiscard]] range remap_range(parser_position s, parser_position e) const noexcept;

    void add_diagnostic(diagnostic_op d) const;
    void add_diagnostic(diagnostic_op (&d)(const range&)) const;

    void syntax_error_or_eof() const;

    void add_hl_symbol(const range& r, hl_scopes s);

    context::id_index parse_identifier(std::string value, range id_range) const;

    context::id_index add_id(std::string value) const;
    context::id_index add_id(std::string_view value) const;

    void lex_last_remark();

    void lex_line_remark();

    void resolve_expression(expressions::ca_expr_ptr& expr) const;

    void resolve_concat_chain(const semantics::concat_chain& chain) const;

    std::string lex_ord();

    std::string lex_ord_upper();

    result_t<context::id_index> lex_id();

    struct qualified_id
    {
        context::id_index qual;
        context::id_index id;
    };

    result_t<qualified_id> lex_qualified_id();

    result_t<std::variant<context::id_index, semantics::concat_chain>> lex_variable_name(parser_position start);
    result_t<semantics::vs_ptr> lex_variable();

    result_t<semantics::concat_chain> lex_compound_variable();

    [[nodiscard]] constexpr bool follows_NOT() const noexcept
    {
        return follows<group<u8'N', u8'n'>, group<u8'O', u8'o'>, group<u8'T', u8't'>>() && input.next[3] != EOF_SYMBOL
            && !char_is_ord(input.next[3]);
    }

    static constexpr auto PROCESS = context::id_index("*PROCESS");
    [[nodiscard]] constexpr bool follows_PROCESS() const noexcept
    {
        return follows<group<u8'*'>,
                   group<u8'P', u8'p'>,
                   group<u8'R', u8'r'>,
                   group<u8'O', u8'o'>,
                   group<u8'C', u8'c'>,
                   group<u8'E', u8'e'>,
                   group<u8'S', u8's'>,
                   group<u8'S', u8's'>>()
            && (input.next[PROCESS.size()] == EOF_SYMBOL || input.next[PROCESS.size()] == u8' ');
    }

    result_t<semantics::seq_sym> lex_seq_symbol();

    result_t<expressions::ca_expr_ptr> lex_expr_general();

    result_t<semantics::concat_chain> lex_ca_string_value();

    result_t<expressions::ca_string::substring_t> lex_substring();

    result_t<std::pair<semantics::concat_chain, expressions::ca_string::substring_t>>
    lex_ca_string_with_optional_substring();

    bool lex_optional_space();

    result_t<std::vector<expressions::ca_expr_ptr>> lex_subscript_ne();

    [[nodiscard]] auto parse_self_def_term(std::string_view type, std::string_view value, range r);
    [[nodiscard]] auto parse_self_def_term_in_mach(std::string_view type, std::string_view value, range r);

    result_t<expressions::ca_expr_ptr> lex_rest_of_ca_string_group(
        expressions::ca_expr_ptr initial_duplicate_factor, const parser_position& start);

    result_t<maybe_expr_list> lex_maybe_expression_list();

    result_t<expressions::ca_expr_ptr> lex_expr_list();

    result_t<expressions::ca_expr_ptr> lex_self_def();

    result_t<expressions::ca_expr_ptr> lex_attribute_reference();

    bool follows_function() const;

    result_t<expressions::ca_expr_ptr> lex_term();

    result_t<std::pair<std::string, range>> lex_number_as_string();

    result_t<std::pair<int32_t, range>> lex_number_as_int();

    result_t<expressions::ca_expr_ptr> lex_num();

    result_t<expressions::mach_expr_ptr> lex_mach_term();

    result_t<std::string> lex_simple_string();

    result_t<expressions::mach_expr_ptr> lex_mach_term_c();

    result_t<expressions::mach_expr_ptr> lex_mach_expr_s();

    result_t<expressions::mach_expr_ptr> lex_mach_expr();

    static bool is_type_extension(char type, char ch)
    {
        return checking::data_def_type::types_and_extensions.contains(std::make_pair(type, ch));
    }

    static constexpr int digit_to_value(char8_t c) noexcept
    {
        static_assert(u8'0' + 0 == u8'0');
        static_assert(u8'0' + 1 == u8'1');
        static_assert(u8'0' + 2 == u8'2');
        static_assert(u8'0' + 3 == u8'3');
        static_assert(u8'0' + 4 == u8'4');
        static_assert(u8'0' + 5 == u8'5');
        static_assert(u8'0' + 6 == u8'6');
        static_assert(u8'0' + 7 == u8'7');
        static_assert(u8'0' + 8 == u8'8');
        static_assert(u8'0' + 9 == u8'9');
        assert(c >= u8'0' && c <= u8'9');
        return c - u8'0';
    }

    result_t<std::pair<int32_t, range>> parse_number();

    result_t<expressions::mach_expr_ptr> lex_literal_signed_num();

    result_t<expressions::mach_expr_ptr> lex_literal_unsigned_num();

    result_t<expressions::data_definition> lex_data_def_base();

    result_t<expressions::expr_or_address> lex_expr_or_addr();

    result_t<expressions::expr_or_address_list> lex_literal_nominal_addr();

    result_t<expressions::nominal_value_ptr> lex_literal_nominal();

    result_t<expressions::data_definition> lex_data_definition(bool require_nominal);

    std::string capture_text(const char8_t* start, const char8_t* end) const;

    std::string capture_text(const char8_t* start) const;

    result_t<semantics::literal_si> lex_literal();

    result_t<expressions::ca_expr_ptr> lex_term_c();

    result_t<expressions::ca_expr_ptr> lex_expr_s();

    result_t<expressions::ca_expr_ptr> lex_expr();

    result_t<std::vector<expressions::ca_expr_ptr>> lex_subscript();

    result_t<void> lex_macro_operand_amp(concat_chain_builder& ccb);

    result_t<void> lex_macro_operand_string(concat_chain_builder& ccb);

    result_t<bool> lex_macro_operand_attr(concat_chain_builder& ccb);

    result_t<void> lex_macro_operand(semantics::concat_chain& cc, bool next_char_special, bool op_name);

    void process_optional_line_remark();

    result_t<void> process_macro_list(std::vector<semantics::concat_chain>& cc);

    result_t<void> handle_initial_space(bool reparse);

    std::pair<semantics::operand_list, range> macro_ops(bool reparse);

    template<std::pair<bool, semantics::operand_ptr> (parser2::*arg)(parser_position)>
    std::pair<semantics::operand_list, range> ca_args();
    std::pair<bool, semantics::operand_ptr> ca_expr_ops(parser_position start);
    std::pair<bool, semantics::operand_ptr> ca_branch_ops(parser_position start);
    std::pair<bool, semantics::operand_ptr> ca_var_def_ops(parser_position start);

    parser_holder::op_data lab_instr();
    void lab_instr_process();
    parser_holder::op_data lab_instr_empty(parser_position start);

    parser_holder::op_data look_lab_instr();
    parser_holder::op_data look_lab_instr_seq();

    result_t<void> lex_label_string(concat_chain_builder& cb);
    result_t<semantics::concat_chain> lex_label();
    result_t<semantics::concat_chain> lex_instr();

    void lex_handle_label(semantics::concat_chain cc, range r);
    void lex_handle_instruction(semantics::concat_chain cc, range r);

    parser_holder::op_data lab_instr_rest();

    std::optional<int> maybe_loctr_len();

    result_t<void> lex_deferred_string(std::vector<semantics::vs_ptr>& vs);
    void op_rem_body_deferred();
    void op_rem_body_noop();

    struct before_model
    {
        bool variable_follows;
        bool next_char_special;
        std::optional<parser_position> in_string;
    };
    result_t<before_model> model_before_variable();

    template<result_t<semantics::operand_ptr> (parser2::*first)(),
        result_t<semantics::operand_ptr> (parser2::*rest)() = first>
    std::optional<semantics::op_rem> with_model(bool reparse, bool model_allowed) requires(first != nullptr);

    result_t<semantics::operand_ptr> mach_op();
    result_t<semantics::operand_ptr> dat_op();

    result_t<semantics::operand_ptr> alias_op();
    result_t<semantics::operand_ptr> end_op();
    result_t<semantics::operand_ptr> using_op1();
    result_t<semantics::operand_ptr> asm_mach_expr();
    result_t<semantics::operand_ptr> asm_op();
    result_t<std::unique_ptr<semantics::complex_assembler_operand::component_value_t>> asm_op_inner();
    result_t<std::vector<std::unique_ptr<semantics::complex_assembler_operand::component_value_t>>> asm_op_comma_c();

    [[nodiscard]] constexpr bool ord_followed_by_parenthesis() const noexcept;

    result_t<void> lex_rest_of_model_string(concat_chain_builder& ccb);
    result_t<std::optional<semantics::op_rem>> try_model_ops(parser_position line_start);

    void lookahead_operands_and_remarks_dat();
    void lookahead_operands_and_remarks_asm();

    explicit parser2(parser_holder* h)
        : holder(h)
        , input(holder->input_state)
        , data(holder->input.data())
    {}
};

class parser2::concat_chain_builder
{
    parser2& p;
    semantics::concat_chain& cc;

    semantics::char_str_conc* last_text_state = nullptr;
    bool highlighting = true;

    template<typename T, hl_scopes... s>
    void single_char_push() requires(sizeof...(s) <= 1)
    {
        push_last_text();
        const auto start = p.cur_pos_adjusted();
        p.consume(s...);
        const auto r = p.range_from(start);
        cc.emplace_back(std::in_place_type<T>, r);
        last_text_state = nullptr;
    }

public:
    concat_chain_builder(parser2& p, semantics::concat_chain& cc, bool hl = true) noexcept
        : p(p)
        , cc(cc)
        , highlighting(hl)
    {}

    [[nodiscard]] std::string& last_text_value()
    {
        if (last_text_state)
            return last_text_state->value;
        last_text_state = &std::get<semantics::char_str_conc>(
            cc.emplace_back(
                  std::in_place_type<semantics::char_str_conc>, std::string(), range(p.cur_pos_adjusted().pos))
                .value);
        return last_text_state->value;
    }

    void push_last_text()
    {
        if (!last_text_state)
            return;
        last_text_state->conc_range = p.remap_range(parser_position { last_text_state->conc_range.start }, p.cur_pos());
        if (highlighting)
            p.add_hl_symbol(last_text_state->conc_range, hl_scopes::operand);
        last_text_state = nullptr;
    }

    template<bool hl = false>
    void push_dot()
    {
        if (highlighting && hl)
            single_char_push<semantics::dot_conc, hl_scopes::operator_symbol>();
        else
            single_char_push<semantics::dot_conc>();
    }

    template<bool hl = false>
    void push_equals()
    {
        if (highlighting && hl)
            single_char_push<semantics::equals_conc, hl_scopes::operator_symbol>();
        else
            single_char_push<semantics::equals_conc>();
    }

    template<typename... Args>
    void emplace_back(Args&&... args)
    {
        push_last_text();
        cc.emplace_back(std::forward<Args>(args)...);
    }
};

std::optional<int> parser2::maybe_loctr_len()
{
    if (!holder->proc_status.has_value())
        return std::nullopt;
    const auto& [_, opcode] = *holder->proc_status;
    return processing::processing_status_cache_key::generate_loctr_len(opcode);
}

[[nodiscard]] constexpr bool parser2::before_nl() const noexcept
{
    return static_cast<size_t>(input.next - data) < *input.nl;
}

constexpr void parser2::adjust_lines() noexcept
{
    if (before_nl())
        return;

    input.char_position_in_line = holder->cont;
    input.char_position_in_line_utf16 = holder->cont;
    do
    {
        ++input.line;
        ++input.nl;
    } while (!before_nl());
    return;
}

constexpr auto utf8_length_extras = []() {
    std::uint32_t v = 0;

    for (size_t i = 0; i < 0x100; i += 16)
    {
        unsigned long long bits = 0;
        if (i <= 0b0111'1111)
            bits = 0;
        else if (0b1100'0000 <= i && i <= 0b1101'1111)
            bits = 1;
        else if (0b1110'0000 <= i && i <= 0b1110'1111)
            bits = 2;
        else if (0b1111'0000 <= i && i <= 0b1111'0111)
            bits = 3;
        v |= bits << (i >> 4 << 1);
    }

    return v;
}();

constexpr char8_t first_long_utf16 = 0xF0;

void parser2::consume() noexcept
{
    assert(!eof());

    const auto ch = *input.next;

    adjust_lines();

    input.next += 1 + (utf8_length_extras >> (ch >> 4 << 1) & 0b11);

    ++input.char_position_in_line;
    input.char_position_in_line_utf16 += 1 + (ch >= first_long_utf16);
}

void parser2::consume(hl_scopes s) noexcept
{
    assert(!eof());

    const auto pos = cur_pos_adjusted();
    consume();

    add_hl_symbol(range_from(pos), s);
}

void parser2::consume_into(std::string& s)
{
    assert(!eof());

    const auto ch = *input.next;

    adjust_lines();

    do
    {
        s.push_back(*input.next);
        ++input.next;
    } while ((*input.next & 0xC0) == 0x80);

    ++input.char_position_in_line;
    input.char_position_in_line_utf16 += 1 + (ch >= first_long_utf16);
}

void parser2::consume_into(std::string& s, hl_scopes scope)
{
    assert(!eof());
    const auto pos = cur_pos_adjusted();
    consume_into(s);
    add_hl_symbol(range_from(pos), scope);
}


[[nodiscard]] parser_position parser2::cur_pos() const noexcept
{
    return { position(input.line, input.char_position_in_line_utf16) };
}

[[nodiscard]] parser_position parser2::cur_pos_adjusted() noexcept
{
    adjust_lines();
    return cur_pos();
}

[[nodiscard]] range parser2::cur_pos_range() const noexcept { return empty_range(cur_pos()); }

[[nodiscard]] range parser2::range_from(parser_position start) const noexcept
{
    const auto end = cur_pos();
    return remap_range(start, end);
}

[[nodiscard]] range parser2::empty_range(parser_position start) const noexcept { return remap_range(start, start); }

void parser2::consume_rest()
{
    while (except<u8' '>())
        consume();
    adjust_lines();
    if (!eof())
        lex_last_remark();
}

[[nodiscard]] range parser2::remap_range(parser_position s, parser_position e) const noexcept
{
    return holder->range_prov.adjust_range({ s.pos, e.pos });
}

void parser2::add_diagnostic(diagnostic_op d) const
{
    if (holder->diagnostic_collector)
        holder->diagnostic_collector->add_diagnostic(std::move(d));
}

void parser2::add_diagnostic(diagnostic_op (&d)(const range&)) const { add_diagnostic(d(cur_pos_range())); }

void parser2::syntax_error_or_eof() const
{
    if (*input.next == EOF_SYMBOL)
        add_diagnostic(diagnostic_op::error_S0003);
    else
        add_diagnostic(diagnostic_op::error_S0002);
}

void parser2::add_hl_symbol(const range& r, hl_scopes s) { holder->collector.add_hl_symbol(token_info(r, s)); }

context::id_index parser2::parse_identifier(std::string value, range id_range) const
{
    if (value.size() > 63 && holder->diagnostic_collector)
        holder->diagnostic_collector->add_diagnostic(diagnostic_op::error_S100(value, id_range));

    return holder->hlasm_ctx->add_id(std::move(value));
}

// TODO: This should be changed, so the id_index is always valid ordinary symbol
context::id_index parser2::add_id(std::string value) const { return holder->hlasm_ctx->add_id(std::move(value)); }
context::id_index parser2::add_id(std::string_view value) const { return holder->hlasm_ctx->add_id(value); }

class [[nodiscard]] parser2::literal_controller
{
    enum class request_t
    {
        none,
        off,
        on,
    } request = request_t::none;
    parser2& impl;

public:
    explicit literal_controller(parser2& impl) noexcept
        : impl(impl)
    {}
    literal_controller(parser2& impl, bool restore) noexcept
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

parser2::literal_controller parser2::enable_literals() noexcept
{
    if (literals_allowed)
        return literal_controller(*this);

    literals_allowed = true;
    return literal_controller(*this, false);
}

parser2::literal_controller parser2::disable_literals() noexcept
{
    if (!literals_allowed)
        return literal_controller(*this);

    literals_allowed = false;
    return literal_controller(*this, true);
}

void parser2::lex_last_remark()
{
    consume_spaces();

    const auto last_remark_start = cur_pos_adjusted();
    while (!eof())
        consume();
    adjust_lines();

    if (const auto last_remark_end = cur_pos(); last_remark_start != last_remark_end)
        remarks.push_back(remap_range(last_remark_start, last_remark_end));
}

void parser2::lex_line_remark()
{
    assert(follows<u8' '>() && before_nl());

    while (follows<u8' '>() && before_nl())
        consume();

    if (before_nl())
    {
        const auto last_remark_start = cur_pos_adjusted();
        while (!eof() && before_nl())
            consume();

        if (const auto remark_end = cur_pos(); last_remark_start != remark_end)
            remarks.push_back(remap_range(last_remark_start, remark_end));
    }
}

void parser2::resolve_expression(expressions::ca_expr_ptr& expr) const
{
    diagnostic_consumer_transform diags([collector = holder->diagnostic_collector](diagnostic_op d) {
        if (collector)
            collector->add_diagnostic(std::move(d));
    });
    using enum context::SET_t_enum;
    auto [_, opcode] = *holder->proc_status;
    using wk = context::well_known;
    if (opcode.value == wk::SETA || opcode.value == wk::ACTR || opcode.value == wk::ASPACE || opcode.value == wk::AGO
        || opcode.value == wk::MHELP)
        expr->resolve_expression_tree({ A_TYPE, A_TYPE, true }, diags);
    else if (opcode.value == wk::SETB)
    {
        if (!expr->is_compatible(expressions::ca_expression_compatibility::setb))
            diags.add_diagnostic(diagnostic_op::error_CE016_logical_expression_parenthesis(expr->expr_range));

        expr->resolve_expression_tree({ B_TYPE, B_TYPE, true }, diags);
    }
    else if (opcode.value == wk::AIF)
    {
        if (!expr->is_compatible(expressions::ca_expression_compatibility::aif))
            diags.add_diagnostic(diagnostic_op::error_CE016_logical_expression_parenthesis(expr->expr_range));

        expr->resolve_expression_tree({ B_TYPE, B_TYPE, true }, diags);
    }
    else if (opcode.value == wk::SETC)
    {
        expr->resolve_expression_tree({ C_TYPE, C_TYPE, true }, diags);
    }
    else if (opcode.value == wk::AREAD)
    {
        // aread operand is just enumeration
    }
    else
    {
        assert(false);
        expr->resolve_expression_tree({ UNDEF_TYPE, UNDEF_TYPE, true }, diags);
    }
}

void parser2::resolve_concat_chain(const semantics::concat_chain& chain) const
{
    diagnostic_consumer_transform diags([collector = holder->diagnostic_collector](diagnostic_op d) {
        if (collector)
            collector->add_diagnostic(std::move(d));
    });
    for (const auto& e : chain)
        e.resolve(diags);
}

std::string parser2::lex_ord()
{
    assert(is_ord_first());

    std::string result;
    do
    {
        consume_into(result);
    } while (is_ord());

    return result;
}

std::string parser2::lex_ord_upper()
{
    assert(is_ord_first());

    std::string result;
    do
    {
        consume_into(result);
    } while (is_ord());

    utils::to_upper(result);

    return result;
}

result_t<context::id_index> parser2::lex_id()
{
    assert(is_ord_first());

    const auto start = cur_pos_adjusted();

    std::string name = lex_ord();

    auto id = parse_identifier(std::move(name), range_from(start));
    if (id.empty())
        return failure;
    else
        return id;
}

result_t<parser2::qualified_id> parser2::lex_qualified_id()
{
    auto [error, id1] = lex_id();
    if (error)
        return failure;

    if (try_consume<u8'.'>(hl_scopes::operator_symbol))
    {
        if (!is_ord_first())
        {
            syntax_error_or_eof();
            return failure;
        }

        auto [error2, id2] = lex_id();
        if (error2)
            return failure;

        return { id1, id2 };
    }

    return { context::id_index(), id1 };
}

result_t<semantics::concat_chain> parser2::lex_compound_variable()
{
    if (!except<u8')'>())
    {
        syntax_error_or_eof();
        return failure;
    }
    semantics::concat_chain result;

    while (!eof())
    {
        switch (*input.next)
        {
            case u8')':
                return result;

            case u8'&': {
                auto [error, var] = lex_variable();
                if (error)
                    return failure;
                result.emplace_back(std::in_place_type<semantics::var_sym_conc>, std::move(var));
                break;
            }

            case u8'.': {
                const auto start = cur_pos_adjusted();
                consume(hl_scopes::operator_symbol);
                result.emplace_back(std::in_place_type<semantics::dot_conc>, range_from(start));
                break;
            }

            default: {
                const auto start = cur_pos_adjusted();
                std::string collected;

                while (except<u8')', u8'&', u8'.'>())
                {
                    consume_into(collected);
                }
                const auto r = range_from(start);
                result.emplace_back(std::in_place_type<semantics::char_str_conc>, std::move(collected), r);
                add_hl_symbol(r, hl_scopes::var_symbol);
                break;
            }
        }
    }
    add_diagnostic(diagnostic_op::error_S0011);
    return failure;
}

result_t<semantics::seq_sym> parser2::lex_seq_symbol()
{
    const auto start = cur_pos_adjusted();
    if (!try_consume<u8'.'>() || !is_ord_first())
    {
        syntax_error_or_eof();
        return failure;
    }
    auto [error, id] = lex_id();
    if (error)
        return failure;
    const auto r = range_from(start);
    add_hl_symbol(r, hl_scopes::seq_symbol);
    return { id, r };
}

result_t<expressions::ca_expr_ptr> parser2::lex_expr_general()
{
    const auto start = cur_pos_adjusted();
    if (!follows_NOT())
        return lex_expr();

    std::vector<expressions::ca_expr_ptr> ca_exprs;
    do
    {
        const auto start_not = cur_pos_adjusted();
        consume();
        consume();
        consume();
        const auto r = range_from(start_not);
        add_hl_symbol(r, hl_scopes::operand);
        ca_exprs.push_back(std::make_unique<expressions::ca_symbol>(context::id_index("NOT"), r));
        lex_optional_space();
    } while (follows_NOT());

    auto [error, e] = lex_expr();
    if (error)
        return failure;

    ca_exprs.push_back(std::move(e));
    return std::make_unique<expressions::ca_expr_list>(std::move(ca_exprs), range_from(start), false);
}

result_t<semantics::concat_chain> parser2::lex_ca_string_value()
{
    assert(follows<u8'\''>());

    semantics::concat_chain cc;
    concat_chain_builder ccb(*this, cc, false);

    consume();
    while (true)
    {
        switch (*input.next)
        {
            case EOF_SYMBOL:
                add_diagnostic(diagnostic_op::error_S0005);
                return failure;

            case u8'.':
                ccb.push_dot();
                break;

            case u8'=':
                ccb.push_equals();
                break;

            case u8'&':
                if (input.next[1] == u8'&')
                {
                    consume_into(ccb.last_text_value());
                    consume_into(ccb.last_text_value());
                }
                else
                {
                    ccb.push_last_text();
                    auto [error, vs] = lex_variable();
                    if (error)
                        return failure;
                    ccb.emplace_back(std::in_place_type<semantics::var_sym_conc>, std::move(vs));
                }
                break;

            case u8'\'':
                consume();
                if (!follows<u8'\''>())
                {
                    ccb.push_last_text();
                    semantics::concatenation_point::clear_concat_chain(cc);
                    return cc;
                }
                [[fallthrough]];

            default: {
                auto& s = ccb.last_text_value();
                do
                {
                    consume_into(s);
                } while (except<u8'.', u8'=', u8'&', u8'\''>());
                break;
            }
        }
    }
}

result_t<expressions::ca_string::substring_t> parser2::lex_substring()
{
    assert(follows<u8'('>());

    const auto sub_start = cur_pos_adjusted();

    consume(hl_scopes::operator_symbol);

    auto [e1_error, e1] = lex_expr_general();
    if (e1_error)
        return failure;

    if (!match<u8','>(hl_scopes::operator_symbol))
        return failure;

    if (try_consume<u8'*'>()) // TODO: no hightlighting?
    {
        if (!match<u8')'>(hl_scopes::operator_symbol, diagnostic_op::error_S0011))
            return failure;
        return { std::move(e1), expressions::ca_expr_ptr(), range_from(sub_start) };
    }

    auto [e2_error, e2] = lex_expr_general();
    if (e2_error)
        return failure;

    if (!match<u8')'>(hl_scopes::operator_symbol, diagnostic_op::error_S0011))
        return failure;

    return {
        std::move(e1),
        std::move(e2),
        range_from(sub_start),
    };
}

result_t<std::pair<semantics::concat_chain, expressions::ca_string::substring_t>>
parser2::lex_ca_string_with_optional_substring()
{
    assert(follows<u8'\''>());
    auto [cc_error, cc] = lex_ca_string_value();
    if (cc_error)
        return failure;

    if (!follows<u8'('>())
        return { std::move(cc), expressions::ca_string::substring_t {} };

    auto [sub_error, sub] = lex_substring();
    if (sub_error)
        return failure;

    return { std::move(cc), std::move(sub) };
}

bool parser2::lex_optional_space()
{
    bool matched = false;
    while (try_consume<u8' '>())
    {
        matched = true;
    }
    return matched;
}

result_t<std::vector<expressions::ca_expr_ptr>> parser2::lex_subscript_ne()
{
    assert(follows<u8'('>());

    std::vector<expressions::ca_expr_ptr> result;

    consume(hl_scopes::operator_symbol);
    if (lex_optional_space())
    {
        auto [error, e] = lex_expr();
        if (error)
            return failure;

        result.push_back(std::move(e));
        lex_optional_space();
        if (!match<u8')'>(hl_scopes::operator_symbol, diagnostic_op::error_S0011))
            return failure;

        return result;
    }

    if (auto [error, e] = lex_expr(); error)
        return failure;
    else
        result.push_back(std::move(e));

    if (lex_optional_space())
    {
        if (!match<u8')'>(hl_scopes::operator_symbol, diagnostic_op::error_S0011))
            return failure;
        return result;
    }
    if (try_consume<u8')'>(hl_scopes::operator_symbol))
        return result;

    if (!match<u8','>(hl_scopes::operator_symbol, diagnostic_op::error_S0002))
        return failure;

    if (auto [error, e] = lex_expr(); error)
        return failure;
    else
        result.push_back(std::move(e));

    while (try_consume<u8','>(hl_scopes::operator_symbol))
    {
        if (auto [error, e] = lex_expr(); error)
            return failure;
        else
            result.push_back(std::move(e));
    }
    if (!match<u8')'>(hl_scopes::operator_symbol, diagnostic_op::error_S0011))
        return failure;

    return result;
}

[[nodiscard]] auto parser2::parse_self_def_term(std::string_view type, std::string_view value, range r)
{
    auto add_diagnostic =
        holder->diagnostic_collector ? diagnostic_adder(*holder->diagnostic_collector, r) : diagnostic_adder(r);
    return expressions::ca_constant::self_defining_term(type, value, add_diagnostic);
}

[[nodiscard]] auto parser2::parse_self_def_term_in_mach(std::string_view type, std::string_view value, range r)
{
    auto add_diagnostic =
        holder->diagnostic_collector ? diagnostic_adder(*holder->diagnostic_collector, r) : diagnostic_adder(r);
    if (type.size() == 1)
    {
        switch (type.front())
        {
            case u8'b':
            case u8'B': {
                if (value.empty())
                    return 0;
                uint32_t res = 0;
                if (auto conv = std::from_chars(value.data(), value.data() + value.size(), res, 2);
                    conv.ec != std::errc() || conv.ptr != value.data() + value.size())
                {
                    add_diagnostic(diagnostic_op::error_CE007);
                    return 0;
                }

                return static_cast<int32_t>(res);
            }
            case u8'd':
            case u8'D': {
                if (value.empty())
                    return 0;

                const auto it = std::ranges::find_if(value, [](auto c) { return c != u8'-' && c != u8'+'; });

                if (it - value.begin() > 1 || (value.front() == u8'-' && value.size() > 11))
                {
                    add_diagnostic(diagnostic_op::error_CE007);
                    return 0;
                }

                size_t start = value.front() == u8'+' ? 1 : 0;

                int32_t res = 0;
                if (auto conv = std::from_chars(value.data() + start, value.data() + value.size(), res, 10);
                    conv.ec != std::errc() || conv.ptr != value.data() + value.size())
                {
                    add_diagnostic(diagnostic_op::error_CE007);
                    return 0;
                }

                return res;
            }
            case u8'x':
            case u8'X': {
                if (value.empty())
                    return 0;
                uint32_t res = 0;
                if (auto conv = std::from_chars(value.data(), value.data() + value.size(), res, 16);
                    conv.ec != std::errc() || conv.ptr != value.data() + value.size())
                {
                    add_diagnostic(diagnostic_op::error_CE007);
                    return 0;
                }

                return static_cast<int32_t>(res);
            }
            default:
                break;
        }
    }
    return expressions::ca_constant::self_defining_term(type, value, add_diagnostic);
}

result_t<expressions::ca_expr_ptr> parser2::lex_rest_of_ca_string_group(
    expressions::ca_expr_ptr initial_duplicate_factor, const parser_position& start)
{
    if (!allow_ca_string())
        return failure;
    auto [error, s] = lex_ca_string_with_optional_substring();
    if (error)
        return failure;

    expressions::ca_expr_ptr result = std::make_unique<expressions::ca_string>(
        std::move(s.first), std::move(initial_duplicate_factor), std::move(s.second), range_from(start));

    while (follows<u8'(', u8'\''>())
    {
        const auto conc_start = cur_pos_adjusted();
        expressions::ca_expr_ptr nested_dupl;
        if (try_consume<u8'('>(hl_scopes::operator_symbol))
        {
            auto [error2, dupl] = lex_expr_general();
            if (error2)
                return failure;
            if (!match<u8')'>(hl_scopes::operator_symbol, diagnostic_op::error_S0011))
                return failure;
            nested_dupl = std::move(dupl);
            if (!follows<u8'\''>())
                return failure;
        }
        auto [error2, s2] = lex_ca_string_with_optional_substring();
        if (error2)
            return failure;

        result = std::make_unique<expressions::ca_basic_binary_operator<expressions::ca_conc>>(std::move(result),
            std::make_unique<expressions::ca_string>(
                std::move(s2.first), std::move(nested_dupl), std::move(s2.second), range_from(conc_start)),
            range_from(start));
    }
    return result;
}

result_t<parser2::maybe_expr_list> parser2::lex_maybe_expression_list()
{
    expressions::ca_expr_ptr p_expr;
    std::vector<expressions::ca_expr_ptr> expr_list;

    auto leading_spaces = lex_optional_space();
    if (auto [error, e] = lex_expr(); error)
        return failure;
    else
        p_expr = std::move(e);

    auto trailing_spaces = lex_optional_space();
    for (; except<u8')'>(); trailing_spaces = lex_optional_space())
    {
        auto [error, e] = lex_expr();
        if (error)
            return failure;
        if (p_expr)
            expr_list.push_back(std::move(p_expr));
        expr_list.push_back(std::move(e));
    }
    const auto lt_spaces = leading_spaces || trailing_spaces;
    if (lt_spaces && p_expr)
        expr_list.push_back(std::move(p_expr));
    if (!expr_list.empty())
        return { std::move(expr_list), lt_spaces };
    else
        return { std::move(p_expr), lt_spaces };
}

result_t<expressions::ca_expr_ptr> parser2::lex_expr_list()
{
    assert(follows<u8'('>());
    const auto start = cur_pos_adjusted();

    consume(hl_scopes::operator_symbol);

    std::vector<expressions::ca_expr_ptr> expr_list;

    (void)lex_optional_space();
    if (auto [error, e] = lex_expr(); error)
        return failure;
    else
        expr_list.push_back(std::move(e));

    (void)lex_optional_space();
    for (; except<u8')'>(); (void)lex_optional_space())
    {
        auto [error, e] = lex_expr();
        if (error)
            return failure;
        expr_list.push_back(std::move(e));
    }
    if (!match<u8')'>(hl_scopes::operator_symbol, diagnostic_op::error_S0011))
        return failure;
    return std::make_unique<expressions::ca_expr_list>(std::move(expr_list), range_from(start), true);
}

result_t<expressions::ca_expr_ptr> parser2::lex_self_def()
{
    assert((follows<selfdef, group<u8'\''>>()));
    const auto start = cur_pos_adjusted();

    const auto c = static_cast<char>(*input.next);
    consume(hl_scopes::self_def_type);
    auto [error, s] = lex_simple_string();
    if (error)
        return failure;

    const auto r = range_from(start);
    return std::make_unique<expressions::ca_constant>(parse_self_def_term(std::string_view(&c, 1), std::move(s), r), r);
}

result_t<expressions::ca_expr_ptr> parser2::lex_attribute_reference()
{
    assert((follows<all_attrs, group<u8'\''>>()));
    const auto start = cur_pos_adjusted();

    const auto attr = context::symbol_attributes::transform_attr(utils::upper_cased[*input.next]);
    consume(hl_scopes::data_attr_type);
    consume(hl_scopes::operator_symbol);

    const auto start_value = cur_pos_adjusted();
    switch (*input.next)
    {
        case u8'&': {
            auto [error, v] = lex_variable();
            if (error)
                return failure;
            // TODO: in reality, this seems to be much more complicated (arbitrary many dots
            // are consumed for *some* attributes)
            // TODO: highlighting
            (void)try_consume<u8'.'>();

            return std::make_unique<expressions::ca_symbol_attribute>(
                std::move(v), attr, range_from(start), range_from(start_value));
        }

        case u8'*':
            add_diagnostic(diagnostic_op::error_S0014);
            return failure;

        case u8'=': {
            auto [error, l] = lex_literal();
            if (error)
                return failure;
            return std::make_unique<expressions::ca_symbol_attribute>(
                std::move(l), attr, range_from(start), range_from(start_value));
        }

        default: {
            if (!is_ord_first())
            {
                syntax_error_or_eof();
                return failure;
            }
            const auto id_start = cur_pos_adjusted();
            auto [error, id] = lex_id();
            if (error)
                return failure;
            const auto id_r = range_from(id_start);
            add_hl_symbol(id_r, hl_scopes::ordinary_symbol);
            return std::make_unique<expressions::ca_symbol_attribute>(id, attr, range_from(start), id_r);
        }
    }
}

bool parser2::follows_function() const
{
    if (!is_ord_first())
        return false;
    const auto* p = input.next;
    std::string s;
    while (char_is_ord(*p))
    {
        if (s.size() >= expressions::ca_common_expr_policy::max_function_name_length)
            return false;
        s.push_back((char)*p);
        ++p;
    }
    return expressions::ca_common_expr_policy::get_function(s) != expressions::ca_expr_funcs::UNKNOWN;
}

result_t<expressions::ca_expr_ptr> parser2::lex_term()
{
    const auto start = cur_pos_adjusted();
    switch (*input.next)
    {
        case u8'&': {
            auto [error, v] = lex_variable();
            if (error)
                return failure;
            return std::make_unique<expressions::ca_var_sym>(std::move(v), range_from(start));
        }

        case u8'-':
        case u8'0':
        case u8'1':
        case u8'2':
        case u8'3':
        case u8'4':
        case u8'5':
        case u8'6':
        case u8'7':
        case u8'8':
        case u8'9':
            return lex_num();

        case u8'\'':
            if (auto [error, s] = lex_rest_of_ca_string_group({}, start); error)
                return failure;
            else
            {
                add_hl_symbol(range_from(start), hl_scopes::string);
                return std::move(s);
            }

        case u8'(': {
            consume(hl_scopes::operator_symbol);

            auto [error, maybe_expr_list] = lex_maybe_expression_list();
            if (error)
                return failure;
            if (!match<u8')'>(hl_scopes::operator_symbol, diagnostic_op::error_S0011))
                return failure;
            expressions::ca_expr_ptr p_expr;
            const auto already_expr_list =
                std::holds_alternative<std::vector<expressions::ca_expr_ptr>>(maybe_expr_list.value);
            if (already_expr_list)
                p_expr = std::make_unique<expressions::ca_expr_list>(
                    std::move(std::get<std::vector<expressions::ca_expr_ptr>>(maybe_expr_list.value)),
                    range_from(start),
                    true);
            else
                p_expr = std::move(std::get<expressions::ca_expr_ptr>(maybe_expr_list.value));

            if (maybe_expr_list.leading_trailing_spaces)
                return p_expr;

            if (follows<u8'\''>())
            {
                if (auto [error2, s] = lex_rest_of_ca_string_group(std::move(p_expr), start); error2)
                    return failure;
                else
                {
                    add_hl_symbol(range_from(start), hl_scopes::string);
                    return std::move(s);
                }
            }
            else if (follows_function())
            {
                auto [id_error, id] = lex_id();
                if (id_error)
                    return failure;
                if (!must_follow<u8'('>())
                    return failure;
                auto [s_error, s] = lex_subscript_ne();
                if (s_error)
                    return failure;
                return std::make_unique<expressions::ca_function>(id,
                    expressions::ca_common_expr_policy::get_function(id.to_string_view()),
                    std::move(s),
                    std::move(p_expr),
                    range_from(start));
            }

            if (!already_expr_list)
            {
                std::vector<expressions::ca_expr_ptr> ops;
                ops.push_back(std::move(p_expr));
                p_expr = std::make_unique<expressions::ca_expr_list>(std::move(ops), range_from(start), true);
            }

            return p_expr;
        }

        default:
            if (!is_ord_first())
            {
                syntax_error_or_eof();
                return failure;
            }

            if (follows<selfdef, group<u8'\''>>())
            {
                auto [error, self_def] = lex_self_def();
                if (error)
                    return failure;
                return std::move(self_def);
            }


            if (follows<all_attrs, group<u8'\''>>())
            {
                auto [error, attr_ref] = lex_attribute_reference();
                if (error)
                    return failure;
                return std::move(attr_ref);
            }

            if (auto [error, id] = lex_id(); error)
                return failure;
            else if (follows<u8'('>()
                && expressions::ca_common_expr_policy::get_function(id.to_string_view())
                    != expressions::ca_expr_funcs::UNKNOWN)
            {
                const auto r = range_from(start);
                add_hl_symbol(r, hl_scopes::operand);
                auto [error2, s] = lex_subscript_ne();
                if (error2)
                    return failure;
                return std::make_unique<expressions::ca_function>(id,
                    expressions::ca_common_expr_policy::get_function(id.to_string_view()),
                    std::move(s),
                    expressions::ca_expr_ptr(),
                    range_from(start));
            }
            else
            {
                const auto r = range_from(start);
                add_hl_symbol(r, hl_scopes::operand);
                return std::make_unique<expressions::ca_symbol>(id, r);
            }
    }
}

result_t<std::pair<std::string, range>> parser2::lex_number_as_string()
{
    assert((follows<u8'0', u8'1', u8'2', u8'3', u8'4', u8'5', u8'6', u8'7', u8'8', u8'9', u8'-'>()));
    const auto start = cur_pos_adjusted();

    std::string result;

    if (follows<u8'-'>())
    {
        consume_into(result);
    }
    if (!is_num())
    {
        syntax_error_or_eof();
        return failure;
    }
    do
    {
        consume_into(result);
    } while (is_num());

    const auto r = range_from(start);
    add_hl_symbol(r, hl_scopes::number);

    return { std::move(result), r };
}

result_t<std::pair<int32_t, range>> parser2::lex_number_as_int()
{
    const auto [error, number] = lex_number_as_string();
    if (error)
        return failure;
    const auto& [v, r] = number;
    return { parse_self_def_term("D", v, r), r };
}

result_t<expressions::ca_expr_ptr> parser2::lex_num()
{
    assert((follows<u8'0', u8'1', u8'2', u8'3', u8'4', u8'5', u8'6', u8'7', u8'8', u8'9', u8'-'>()));
    const auto [error, number] = lex_number_as_int();
    if (error)
        return failure;
    const auto& [v, r] = number;
    return std::make_unique<expressions::ca_constant>(v, r);
}

result_t<expressions::mach_expr_ptr> parser2::lex_mach_term()
{
    const auto start = cur_pos_adjusted();
    switch (*input.next)
    {
        case EOF_SYMBOL:
            add_diagnostic(diagnostic_op::error_S0003);
            return failure;

        case u8'(': {
            consume(hl_scopes::operator_symbol);
            auto [error, e] = lex_mach_expr();
            if (error)
                return failure;
            if (!match<u8')'>(hl_scopes::operator_symbol, diagnostic_op::error_S0011))
                return failure;
            return std::make_unique<expressions::mach_expr_unary<expressions::par>>(std::move(e), range_from(start));
        }

        case u8'*':
            consume(hl_scopes::operand);
            return std::make_unique<expressions::mach_expr_location_counter>(range_from(start));

        case u8'-':
        case u8'0':
        case u8'1':
        case u8'2':
        case u8'3':
        case u8'4':
        case u8'5':
        case u8'6':
        case u8'7':
        case u8'8':
        case u8'9': {
            const auto [error, number] = lex_number_as_string();
            if (error)
                return failure;
            const auto& [v, r] = number;
            return std::make_unique<expressions::mach_expr_constant>(parse_self_def_term_in_mach("D", v, r), r);
        }

        case u8'=': {
            auto [error, l] = lex_literal();
            if (error)
                return failure;
            return std::make_unique<expressions::mach_expr_literal>(range_from(start), std::move(l));
        }

        default:
            if (!is_ord_first())
            {
                syntax_error_or_eof();
                return failure;
            }
            if (follows<group<u8'L', u8'l'>, group<u8'\''>, group<u8'*'>>())
            {
                consume(hl_scopes::data_attr_type);
                consume(hl_scopes::operator_symbol);
                auto loctr_len = maybe_loctr_len();
                if (!loctr_len.has_value())
                {
                    add_diagnostic(diagnostic_op::error_S0014);
                    return failure;
                }
                consume(hl_scopes::operand);
                return std::make_unique<expressions::mach_expr_constant>(loctr_len.value(), range_from(start));
            }
            if (follows<mach_attrs, group<u8'\''>>())
            {
                const auto attr = context::symbol_attributes::transform_attr(utils::upper_cased[*input.next]);
                consume(hl_scopes::data_attr_type);
                consume(hl_scopes::operator_symbol);
                const auto start_value = cur_pos_adjusted();
                if (follows<u8'='>())
                {
                    auto lit = enable_literals();
                    auto [error, l] = lex_literal();
                    if (error)
                        return failure;

                    return std::make_unique<expressions::mach_expr_data_attr_literal>(
                        std::make_unique<expressions::mach_expr_literal>(range_from(start_value), std::move(l)),
                        attr,
                        range_from(start),
                        range_from(start_value));
                }
                if (is_ord_first())
                {
                    auto [error, q_id] = lex_qualified_id();
                    if (error)
                        return failure;
                    const auto r = range_from(start_value);
                    add_hl_symbol(r, hl_scopes::ordinary_symbol);
                    return std::make_unique<expressions::mach_expr_data_attr>(
                        q_id.id, q_id.qual, attr, range_from(start), r);
                }
                syntax_error_or_eof();
                return failure;
            }
            if (follows<group<u8'C', u8'c'>, group<u8'A', u8'E', u8'U', u8'a', u8'e', u8'u'>, group<u8'\''>>())
            {
                const char opt[2] = { static_cast<char>(input.next[0]), static_cast<char>(input.next[1]) };
                consume();
                consume();
                add_hl_symbol(range_from(start), hl_scopes::self_def_type);
                auto [error, s] = lex_simple_string();
                if (error)
                    return failure;

                const auto r = range_from(start);
                return std::make_unique<expressions::mach_expr_constant>(
                    parse_self_def_term_in_mach(std::string_view(opt, 2), s, r), r);
            }
            if (follows<selfdef, group<u8'\''>>())
            {
                const auto opt = static_cast<char>(*input.next);
                consume(hl_scopes::self_def_type);
                auto [error, s] = lex_simple_string();
                if (error)
                    return failure;

                const auto r = range_from(start);
                return std::make_unique<expressions::mach_expr_constant>(
                    parse_self_def_term_in_mach(std::string_view(&opt, 1), s, r), r);
            }
            if (auto [error, qual_id] = lex_qualified_id(); error)
                return failure;
            else
            {
                const auto r = range_from(start);
                add_hl_symbol(r, hl_scopes::ordinary_symbol);
                return std::make_unique<expressions::mach_expr_symbol>(qual_id.id, qual_id.qual, r);
            }
    }
}

result_t<std::string> parser2::lex_simple_string()
{
    assert(follows<u8'\''>());

    const auto start = cur_pos_adjusted();
    std::string s;

    consume();

    while (!eof())
    {
        if (follows<group<u8'\''>, group<u8'\''>>() || follows<group<u8'&'>, group<u8'&'>>())
        {
            consume_into(s);
            consume();
        }
        else if (follows<u8'\''>())
        {
            consume();
            add_hl_symbol(range_from(start), hl_scopes::string);
            return s;
        }
        else if (follows<u8'&'>())
        {
            add_diagnostic(diagnostic_op::error_S0002);
            return failure;
        }
        else
        {
            consume_into(s);
        }
    }

    add_diagnostic(diagnostic_op::error_S0005);
    return failure;
}

result_t<expressions::mach_expr_ptr> parser2::lex_mach_term_c()
{
    if (follows<u8'+'>() || (follows<u8'-'>() && !char_is_num(input.next[1])))
    {
        const auto plus = *input.next == u8'+';
        const auto start = cur_pos_adjusted();
        consume(hl_scopes::operator_symbol);
        auto [error, e] = lex_mach_term_c();
        if (error)
            return failure;
        if (plus)
            return std::make_unique<expressions::mach_expr_unary<expressions::add>>(std::move(e), range_from(start));
        else
            return std::make_unique<expressions::mach_expr_unary<expressions::sub>>(std::move(e), range_from(start));
    }

    return lex_mach_term();
}

result_t<expressions::mach_expr_ptr> parser2::lex_mach_expr_s()
{
    const auto start = cur_pos_adjusted();
    if (auto [error, e] = lex_mach_term_c(); error)
        return failure;
    else
    {
        while (follows<u8'*', u8'/'>())
        {
            const auto mul = *input.next == u8'*';
            consume(hl_scopes::operator_symbol);
            auto [error2, next] = lex_mach_term_c();
            if (error2)
                return failure;
            if (mul)
                e = std::make_unique<expressions::mach_expr_binary<expressions::mul>>(
                    std::move(e), std::move(next), range_from(start));
            else
                e = std::make_unique<expressions::mach_expr_binary<expressions::div>>(
                    std::move(e), std::move(next), range_from(start));
        }
        return std::move(e);
    }
}

result_t<expressions::mach_expr_ptr> parser2::lex_mach_expr()
{
    const auto start = cur_pos_adjusted();
    if (auto [error, e] = lex_mach_expr_s(); error)
        return failure;
    else
    {
        while (follows<u8'+', u8'-'>())
        {
            const auto plus = *input.next == u8'+';
            consume(hl_scopes::operator_symbol);
            auto [error2, next] = lex_mach_expr_s();
            if (error2)
                return failure;
            if (plus)
                e = std::make_unique<expressions::mach_expr_binary<expressions::add>>(
                    std::move(e), std::move(next), range_from(start));
            else
                e = std::make_unique<expressions::mach_expr_binary<expressions::sub>>(
                    std::move(e), std::move(next), range_from(start));
        }
        return std::move(e);
    }
}

result_t<std::pair<int32_t, range>> parser2::parse_number()
{
    constexpr long long min_l = -(1LL << 31);
    constexpr long long max_l = (1LL << 31) - 1;
    constexpr long long parse_limit_l = (1LL << 31);
    static_assert(std::numeric_limits<int32_t>::min() <= min_l);
    static_assert(std::numeric_limits<int32_t>::max() >= max_l);

    const auto start = cur_pos_adjusted();

    long long result = 0;
    const bool negative = [&]() {
        switch (*input.next)
        {
            case u8'-':
                consume();
                return true;
            case u8'+':
                consume();
                return false;
            default:
                return false;
        }
    }();

    bool parsed_one = false;
    while (is_num())
    {
        const auto c = *input.next;
        parsed_one = true;

        consume();

        if (result > parse_limit_l)
            continue;

        result = result * 10 + digit_to_value(c);
    }
    const auto r = range_from(start);
    if (!parsed_one)
    {
        add_diagnostic(diagnostic_op::error_D002(r));
        return failure;
    }
    if (negative)
        result = -result;
    if (result < min_l || result > max_l)
    {
        add_diagnostic(diagnostic_op::error_D001(r));
        return failure;
    }
    add_hl_symbol(r, hl_scopes::number);

    return { (int32_t)result, r };
}

result_t<expressions::mach_expr_ptr> parser2::lex_literal_signed_num()
{
    if (try_consume<u8'('>(hl_scopes::operator_symbol))
    {
        auto [error, e] = lex_mach_expr();
        if (error)
            return failure;
        if (!match<u8')'>(hl_scopes::operator_symbol, diagnostic_op::error_S0011))
            return failure;
        else
            return std::move(e);
    }
    auto [error, n] = parse_number();
    if (error)
        return failure;
    return std::make_unique<expressions::mach_expr_constant>(n.first, n.second);
}

result_t<expressions::mach_expr_ptr> parser2::lex_literal_unsigned_num()
{
    if (try_consume<u8'('>(hl_scopes::operator_symbol))
    {
        auto [error, e] = lex_mach_expr();
        if (error)
            return failure;
        if (!match<u8')'>(hl_scopes::operator_symbol, diagnostic_op::error_S0011))
            return failure;
        return std::move(e);
    }
    if (!is_num())
    {
        syntax_error_or_eof();
        return failure;
    }
    auto [error, n] = parse_number();
    if (error)
        return failure;
    return std::make_unique<expressions::mach_expr_constant>(n.first, n.second);
}

result_t<expressions::data_definition> parser2::lex_data_def_base()
{
    expressions::data_definition result;
    // duplicating_factor
    if (follows<u8'('>() || is_num())
    {
        if (auto [error, e] = lex_literal_unsigned_num(); error)
            return failure;
        else
            result.dupl_factor = std::move(e);
    }

    // read_type
    if (!is_ord_first())
    {
        syntax_error_or_eof();
        return failure;
    }
    const auto type = utils::upper_cased[*input.next];
    const auto type_start = cur_pos_adjusted();
    consume();

    result.type = type == 'R' && !holder->hlasm_ctx->goff() ? 'r' : type;
    result.type_range = range_from(type_start);
    if (is_ord_first() && is_type_extension(type, utils::upper_cased[*input.next]))
    {
        result.extension = utils::upper_cased[*input.next];
        const auto ext_start = cur_pos_adjusted();
        consume();
        result.extension_range = range_from(ext_start);
    }
    add_hl_symbol(range_from(type_start), hl_scopes::data_def_type);

    // read_program
    if (try_consume<u8'P', u8'p'>(hl_scopes::data_def_modifier))
    {
        auto [error, e] = lex_literal_signed_num();
        if (error)
            return failure;
        result.program_type = std::move(e);
    }

    // read_length
    if (try_consume<u8'L', u8'l'>(hl_scopes::data_def_modifier))
    {
        if (try_consume<u8'.'>())
        {
            result.length_type = expressions::data_definition::length_type::BIT;
        }
        auto [error, e] = lex_literal_unsigned_num();
        if (error)
            return failure;
        result.length = std::move(e);
    }

    // read_scale
    if (try_consume<u8'S', u8's'>(hl_scopes::data_def_modifier))
    {
        auto [error, e] = lex_literal_signed_num();
        if (error)
            return failure;
        result.scale = std::move(e);
    }

    // read_exponent
    using can_have_exponent = decltype(group_from_string<{ u8"DEFHLdefhl" }>());
    if (can_have_exponent::matches(result.type) && try_consume<u8'E', u8'e'>(hl_scopes::data_def_modifier))
    {
        auto [error, e] = lex_literal_signed_num();
        if (error)
            return failure;
        result.exponent = std::move(e);
    }
    return result;
}

result_t<expressions::expr_or_address> parser2::lex_expr_or_addr()
{
    const auto start = cur_pos_adjusted();
    auto [error, e] = lex_mach_expr();
    if (error)
        return failure;

    if (!try_consume<u8'('>(hl_scopes::operator_symbol))
        return { std::move(e) };
    auto [error2, e2] = lex_mach_expr();
    if (error2)
        return failure;
    if (!match<u8')'>(hl_scopes::operator_symbol, diagnostic_op::error_S0011))
        return failure;
    return expressions::expr_or_address(
        std::in_place_type<expressions::address_nominal>, std::move(e), std::move(e2), range_from(start));
}

result_t<expressions::expr_or_address_list> parser2::lex_literal_nominal_addr()
{
    assert(follows<u8'('>());
    consume(hl_scopes::operator_symbol);

    expressions::expr_or_address_list result;

    auto [error, e] = lex_expr_or_addr();
    if (error)
        return failure;
    result.push_back(std::move(e));

    while (try_consume<u8','>(hl_scopes::operator_symbol))
    {
        auto [error2, e_next] = lex_expr_or_addr();
        if (error2)
            return failure;
        result.push_back(std::move(e_next));
    }

    if (!match<u8')'>(hl_scopes::operator_symbol, diagnostic_op::error_S0011))
        return failure;

    return result;
}

result_t<expressions::nominal_value_ptr> parser2::lex_literal_nominal()
{
    const auto start = cur_pos_adjusted();
    if (follows<u8'\''>())
    {
        auto [error, n] = lex_simple_string();
        if (error)
            return failure;
        return std::make_unique<expressions::nominal_value_string>(std::move(n), range_from(start));
    }
    else if (follows<u8'('>())
    {
        auto [error, n] = lex_literal_nominal_addr();
        if (error)
            return failure;
        return std::make_unique<expressions::nominal_value_exprs>(std::move(n));
    }
    else
    {
        syntax_error_or_eof();
        return failure;
    }
}

result_t<expressions::data_definition> parser2::lex_data_definition(bool require_nominal)
{
    auto [error, d] = lex_data_def_base();
    if (error)
        return failure;
    if (require_nominal || follows<u8'(', u8'\''>())
    {
        auto [error2, n] = lex_literal_nominal();
        if (error2)
            return failure;
        d.nominal_value = std::move(n);
    }

    struct loctr_reference_visitor final : public expressions::mach_expr_visitor
    {
        bool found_loctr_reference = false;

        void visit(const expressions::mach_expr_constant&) override {}
        void visit(const expressions::mach_expr_data_attr&) override {}
        void visit(const expressions::mach_expr_data_attr_literal&) override {}
        void visit(const expressions::mach_expr_symbol&) override {}
        void visit(const expressions::mach_expr_location_counter&) override { found_loctr_reference = true; }
        void visit(const expressions::mach_expr_default&) override {}
        void visit(const expressions::mach_expr_literal& expr) override { expr.get_data_definition().apply(*this); }
    } v;
    d.apply(v);
    d.references_loctr = v.found_loctr_reference;

    return std::move(d);
}

std::string parser2::capture_text(const char8_t* start, const char8_t* end) const { return std::string(start, end); }

std::string parser2::capture_text(const char8_t* start) const { return capture_text(start, input.next); }

result_t<semantics::literal_si> parser2::lex_literal()
{
    const auto allowed = are_literals_allowed();
    const auto disabled = disable_literals();
    const auto start = cur_pos_adjusted();
    const auto initial = input.next;

    assert(follows<u8'='>());
    consume(hl_scopes::operator_symbol);

    auto [error, dd] = lex_data_definition(true);
    if (error)
        return failure;

    if (!allowed)
    {
        add_diagnostic(diagnostic_op::error_S0013);
        // continue processing
    }

    return holder->collector.add_literal(capture_text(initial), std::move(dd), range_from(start));
}

result_t<expressions::ca_expr_ptr> parser2::lex_term_c()
{
    if (follows<u8'+'>() || (follows<u8'-'>() && !char_is_num(input.next[1])))
    {
        const auto start = cur_pos_adjusted();
        const auto plus = *input.next == u8'+';
        consume(hl_scopes::operator_symbol);
        auto [error, e] = lex_term_c();
        if (error)
            return failure;
        if (plus)
            return std::make_unique<expressions::ca_plus_operator>(std::move(e), range_from(start));
        else
            return std::make_unique<expressions::ca_minus_operator>(std::move(e), range_from(start));
    }
    return lex_term();
}

result_t<expressions::ca_expr_ptr> parser2::lex_expr_s()
{
    expressions::ca_expr_ptr result;
    const auto start = cur_pos_adjusted();
    auto [error, e] = lex_term_c();
    if (error)
        return failure;
    result = std::move(e);

    while (follows<u8'*', u8'/'>())
    {
        const auto mult = *input.next == u8'*';
        consume(hl_scopes::operator_symbol);
        auto [error2, e_next] = lex_term_c();
        if (error2)
            return failure;
        if (mult)
            result = std::make_unique<expressions::ca_basic_binary_operator<expressions::ca_mul>>(
                std::move(result), std::move(e_next), range_from(start));
        else
            result = std::make_unique<expressions::ca_basic_binary_operator<expressions::ca_div>>(
                std::move(result), std::move(e_next), range_from(start));
    }

    return result;
}

result_t<expressions::ca_expr_ptr> parser2::lex_expr()
{
    expressions::ca_expr_ptr result;
    const auto start = cur_pos_adjusted();

    if (auto [error, e] = lex_expr_s(); error)
        return failure;
    else
        result = std::move(e);

    switch (*input.next)
    {
        case u8'+':
        case u8'-':
            while (follows<u8'+', u8'-'>())
            {
                const auto plus = *input.next == u8'+';
                consume(hl_scopes::operator_symbol);
                auto [error, e] = lex_expr_s();
                if (error)
                    return failure;
                if (plus)
                    result = std::make_unique<expressions::ca_basic_binary_operator<expressions::ca_add>>(
                        std::move(result), std::move(e), range_from(start));
                else
                    result = std::make_unique<expressions::ca_basic_binary_operator<expressions::ca_sub>>(
                        std::move(result), std::move(e), range_from(start));
            }
            break;
        case u8'.':
            while (try_consume<u8'.'>(hl_scopes::operator_symbol))
            {
                auto [error, e] = lex_term_c();
                if (error)
                    return failure;
                result = std::make_unique<expressions::ca_basic_binary_operator<expressions::ca_conc>>(
                    std::move(result), std::move(e), range_from(start));
            }
            break;
    }

    return result;
}

result_t<std::vector<expressions::ca_expr_ptr>> parser2::lex_subscript()
{
    assert(follows<u8'('>());

    consume(hl_scopes::operator_symbol);

    std::vector<expressions::ca_expr_ptr> result;

    auto [error, expr] = lex_expr();
    if (error)
        return failure;
    result.push_back(std::move(expr));

    while (try_consume<u8','>(hl_scopes::operator_symbol))
    {
        auto [error2, expr_next] = lex_expr();
        if (error2)
            return failure;
        result.push_back(std::move(expr_next));
    }

    if (!match<u8')'>(hl_scopes::operator_symbol, diagnostic_op::error_S0011))
        return failure;

    return result;
}

result_t<void> parser2::lex_macro_operand_amp(concat_chain_builder& ccb)
{
    assert(follows<u8'&'>());
    if (input.next[1] == u8'&')
    {
        consume_into(ccb.last_text_value());
        consume_into(ccb.last_text_value());
    }
    else
    {
        ccb.push_last_text();
        auto [error, vs] = lex_variable();
        if (error)
            return failure;
        ccb.emplace_back(std::in_place_type<semantics::var_sym_conc>, std::move(vs));
    }
    return {};
}

result_t<void> parser2::lex_macro_operand_string(concat_chain_builder& ccb)
{
    assert(follows<u8'\''>());

    consume_into(ccb.last_text_value());
    while (true)
    {
        switch (*input.next)
        {
            case EOF_SYMBOL:
                ccb.push_last_text();
                add_diagnostic(diagnostic_op::error_S0005);
                return failure;

            case u8'&':
                if (auto [error] = lex_macro_operand_amp(ccb); error)
                    return failure;
                break;

            case u8'=':
                ccb.push_equals();
                break;

            case u8'.':
                ccb.push_dot();
                break;

            case u8'\'':
                consume_into(ccb.last_text_value());
                if (!follows<u8'\''>())
                {
                    ccb.push_last_text();
                    return {};
                }
                [[fallthrough]];

            default: {
                auto& s = ccb.last_text_value();
                do
                {
                    consume_into(s);
                } while (except<u8'&', u8'=', u8'.', u8'\''>());
                break;
            }
        }
    }
}

result_t<bool> parser2::lex_macro_operand_attr(concat_chain_builder& ccb)
{
    if (input.next[1] != u8'\'')
    {
        consume_into(ccb.last_text_value());
        return false;
    }

    if (char_is_ord_first(input.next[2]) || input.next[2] == u8'=')
    {
        consume_into(ccb.last_text_value());
        consume_into(ccb.last_text_value());
        return false;
    }

    if (input.next[2] != u8'&')
    {
        consume_into(ccb.last_text_value());
        return false;
    }

    while (except<u8',', u8')', u8' '>())
    {
        if (!follows<u8'&'>())
        {
            consume_into(ccb.last_text_value());
        }
        else if (input.next[1] == u8'&')
        {
            consume_into(ccb.last_text_value());
            consume_into(ccb.last_text_value());
        }
        else if (auto [error, vs] = (ccb.push_last_text(), lex_variable()); error)
            return failure;
        else
        {
            ccb.emplace_back(std::in_place_type<semantics::var_sym_conc>, std::move(vs));
            if (follows<u8'.'>())
                ccb.push_dot<true>();
        }
    }
    return true;
}

result_t<void> parser2::lex_macro_operand(semantics::concat_chain& cc, bool next_char_special, bool op_name)
{
    concat_chain_builder ccb(*this, cc);
    while (true)
    {
        const bool last_char_special = std::exchange(next_char_special, true);
        switch (*input.next)
        {
            case EOF_SYMBOL:
            case u8' ':
            case u8')':
            case u8',':
                ccb.push_last_text();
                return {};

            case u8'=':
                ccb.push_equals<true>();
                next_char_special = false;
                break;

            case u8'.':
                ccb.push_dot<true>();
                break;

            case u8'(': {
                std::vector<semantics::concat_chain> nested;
                ccb.push_last_text();
                if (auto [error] = process_macro_list(nested); error)
                    return failure;
                ccb.emplace_back(std::in_place_type<semantics::sublist_conc>, std::move(nested));
                break;
            }

            case u8'\'':
                if (auto [error] = lex_macro_operand_string(ccb); error)
                    return failure;

                next_char_special = false;
                break;

            case u8'&': {
                if (auto [error] = lex_macro_operand_amp(ccb); error)
                    return failure;
                if (op_name && follows<u8'='>())
                    ccb.push_equals<true>();
                else
                    next_char_special = false;
                break;
            }

            case u8'O':
            case u8'S':
            case u8'I':
            case u8'L':
            case u8'T':
            case u8'o':
            case u8's':
            case u8'i':
            case u8'l':
            case u8't':
                if (!last_char_special)
                {
                    consume_into(ccb.last_text_value());
                    next_char_special = false;
                    break;
                }
                if (auto [error, ncs] = lex_macro_operand_attr(ccb); error)
                    return failure;
                else
                    next_char_special = ncs;
                break;

            default:
                next_char_special = !is_ord();
                consume_into(ccb.last_text_value());
                break;
        }
        op_name = false;
    }
}

void parser2::process_optional_line_remark()
{
    if (follows<u8' '>() && before_nl())
    {
        lex_line_remark();

        adjust_lines();
    }
}

result_t<void> parser2::process_macro_list(std::vector<semantics::concat_chain>& cc)
{
    assert(follows<u8'('>());

    consume(hl_scopes::operator_symbol);
    if (try_consume<u8')'>(hl_scopes::operator_symbol))
        return {};

    if (auto [error] = lex_macro_operand(cc.emplace_back(), true, false); error)
        return failure;

    while (try_consume<u8','>(hl_scopes::operator_symbol))
    {
        process_optional_line_remark();
        if (auto [error] = lex_macro_operand(cc.emplace_back(), true, false); error)
            return failure;
    }

    if (!match<u8')'>(hl_scopes::operator_symbol, diagnostic_op::error_S0011))
        return failure;

    return {};
}

result_t<void> parser2::handle_initial_space(bool reparse)
{
    if (!reparse && *input.next != u8' ')
    {
        add_diagnostic(diagnostic_op::error_S0002);
        consume_rest();
        return failure;
    }

    consume_spaces();
    adjust_lines();

    return {};
}

std::pair<semantics::operand_list, range> parser2::macro_ops(bool reparse)
{
    const auto input_start = cur_pos_adjusted();
    if (eof())
        return { semantics::operand_list(), empty_range(input_start) };

    if (auto [error] = handle_initial_space(reparse); error)
        return { semantics::operand_list(), range_from(input_start) };

    if (eof())
        return { semantics::operand_list(), cur_pos_range() };

    semantics::operand_list result;

    auto line_start = cur_pos_adjusted();
    auto start = line_start;
    semantics::concat_chain cc;
    bool pending = true;

    const auto push_operand = [&]() {
        if (!pending)
            return;
        if (cc.empty())
            result.push_back(std::make_unique<semantics::empty_operand>(range_from(start)));
        else
            result.push_back(std::make_unique<semantics::macro_operand>(std::move(cc), range_from(start)));
    };

    // process operands
    while (!eof())
    {
        switch (*input.next)
        {
            case u8' ':
                push_operand();
                pending = false;
                lex_last_remark();
                goto end;

            case u8',':
                push_operand();
                consume(hl_scopes::operator_symbol);
                process_optional_line_remark();
                start = cur_pos_adjusted();
                break;

            case u8'O':
            case u8'S':
            case u8'I':
            case u8'L':
            case u8'T':
            case u8'o':
            case u8's':
            case u8'i':
            case u8'l':
            case u8't':
                if (input.next[1] == u8'\'')
                {
                    if (auto [error] = lex_macro_operand(cc, true, false); error)
                    {
                        consume_rest();
                        goto end;
                    }
                    break;
                }
                [[fallthrough]];

            case u8'$':
            case u8'_':
            case u8'#':
            case u8'@':
            case u8'a':
            case u8'b':
            case u8'c':
            case u8'd':
            case u8'e':
            case u8'f':
            case u8'g':
            case u8'h':
            case u8'j':
            case u8'k':
            case u8'm':
            case u8'n':
            case u8'p':
            case u8'q':
            case u8'r':
            case u8'u':
            case u8'v':
            case u8'w':
            case u8'x':
            case u8'y':
            case u8'z':
            case u8'A':
            case u8'B':
            case u8'C':
            case u8'D':
            case u8'E':
            case u8'F':
            case u8'G':
            case u8'H':
            case u8'J':
            case u8'K':
            case u8'M':
            case u8'N':
            case u8'P':
            case u8'Q':
            case u8'R':
            case u8'U':
            case u8'V':
            case u8'W':
            case u8'X':
            case u8'Y':
            case u8'Z': {
                bool next_char_special = false;
                concat_chain_builder ccb(*this, cc);
                auto& l = ccb.last_text_value();
                do
                {
                    l.push_back(static_cast<char>(*input.next));
                    consume();
                } while (is_ord());
                ccb.push_last_text();
                if (follows<u8'='>())
                {
                    ccb.push_equals(); // TODO: no highlighting???
                    next_char_special = true;
                }
                if (const auto n = *input.next; n == EOF_SYMBOL || n == u8' ' || n == u8',')
                    continue;
                if (auto [error] = lex_macro_operand(cc, next_char_special, false); error)
                {
                    consume_rest();
                    goto end;
                }
                break;
            }

            case u8'&':
                if (auto [error] = lex_macro_operand(cc, true, true); error)
                {
                    consume_rest();
                    goto end;
                }
                break;

            case u8'\'':
            default:
                if (auto [error] = lex_macro_operand(cc, true, false); error)
                {
                    consume_rest();
                    goto end;
                }
                break;

            case u8')':
                add_diagnostic(diagnostic_op::error_S0012);
                consume_rest();
                goto end;

            case u8'(': {
                std::vector<semantics::concat_chain> nested;
                if (auto [error] = process_macro_list(nested); error)
                {
                    consume_rest();
                    goto end;
                }
                cc.emplace_back(std::in_place_type<semantics::sublist_conc>, std::move(nested));
                break;
            }
        }
    }
end:;
    push_operand();

    return { std::move(result), range_from(line_start) };
}

result_t<std::variant<context::id_index, semantics::concat_chain>> parser2::lex_variable_name(parser_position start)
{
    if (follows<u8'('>())
    {
        add_hl_symbol(range_from(start), hl_scopes::var_symbol);
        consume(hl_scopes::operator_symbol);
        auto [error, cc] = lex_compound_variable();
        if (error)
            return failure;
        if (!match<u8')'>(hl_scopes::operator_symbol, diagnostic_op::error_S0011))
            return failure;
        return std::move(cc);
    }
    else if (!is_ord_first())
    {
        add_diagnostic(diagnostic_op::error_S0008);
        return failure;
    }
    else
    {
        auto [error, id] = lex_id();
        if (error)
            return failure;
        add_hl_symbol(range_from(start), hl_scopes::var_symbol);
        return id;
    }
}

result_t<semantics::vs_ptr> parser2::lex_variable()
{
    assert(follows<u8'&'>());

    const auto start = cur_pos_adjusted();
    consume();

    auto [error, var_name] = lex_variable_name(start);
    if (error)
        return failure;

    std::vector<expressions::ca_expr_ptr> sub;
    if (follows<u8'('>())
    {
        if (auto [error_sub, sub_] = lex_subscript(); error_sub)
            return failure;
        else
            sub = std::move(sub_);
    }

    const auto r = range_from(start);

    if (std::holds_alternative<context::id_index>(var_name))
        return std::make_unique<semantics::basic_variable_symbol>(
            std::get<context::id_index>(var_name), std::move(sub), r);
    else
    {
        auto& cc = std::get<semantics::concat_chain>(var_name);
        return std::make_unique<semantics::created_variable_symbol>(std::move(cc), std::move(sub), r);
    }
}

template<std::pair<bool, semantics::operand_ptr> (parser2::*arg)(parser_position)>
std::pair<semantics::operand_list, range> parser2::ca_args()
{
    if (const auto input_start = cur_pos_adjusted(); eof())
        return { semantics::operand_list(), empty_range(input_start) };
    else if (!lex_optional_space())
    {
        syntax_error_or_eof();
        consume_rest();
        return { semantics::operand_list(), empty_range(input_start) };
    }
    else if (eof())
        return { semantics::operand_list(), cur_pos_range() };

    const auto line_start = cur_pos_adjusted();

    semantics::operand_list result;

    bool pending = true;
    while (except<u8' '>())
    {
        const auto start = cur_pos_adjusted();
        if (try_consume<u8','>(hl_scopes::operator_symbol))
        {
            if (pending)
                result.push_back(std::make_unique<semantics::empty_operand>(empty_range(start)));
            process_optional_line_remark();
            pending = true;
            continue;
        }
        else if (!pending)
        {
            syntax_error_or_eof();
            break;
        }
        auto [continue_loop, op] = (this->*arg)(start);
        if (op)
            result.push_back(std::move(op));
        else
            result.push_back(std::make_unique<semantics::empty_operand>(empty_range(start)));
        pending = false;

        if (!continue_loop)
            break;
    }
    if (pending)
        result.push_back(std::make_unique<semantics::empty_operand>(cur_pos_range()));

    consume_rest();

    return { std::move(result), range_from(line_start) };
}

std::pair<bool, semantics::operand_ptr> parser2::ca_expr_ops(parser_position start)
{
    auto [error, expr] = lex_expr_general();
    if (error)
    {
        const auto r = range_from(start);
        // original fallback
        return {
            false,
            std::make_unique<semantics::expr_ca_operand>(std::make_unique<expressions::ca_constant>(0, r), r),
        };
    }
    resolve_expression(expr);
    return { true, std::make_unique<semantics::expr_ca_operand>(std::move(expr), range_from(start)) };
}

std::pair<bool, semantics::operand_ptr> parser2::ca_branch_ops(parser_position start)
{
    expressions::ca_expr_ptr first_expr;
    if (follows<u8'('>())
    {
        auto [error, e] = lex_expr_list();
        if (error)
            return {};
        first_expr = std::move(e);
        resolve_expression(first_expr);
    }
    auto [error, ss] = lex_seq_symbol();
    if (error)
        return {};
    const auto r = range_from(start);
    if (first_expr)
        return { true, std::make_unique<semantics::branch_ca_operand>(std::move(ss), std::move(first_expr), r) };
    else
        return { true, std::make_unique<semantics::seq_ca_operand>(std::move(ss), r) };
}

std::pair<bool, semantics::operand_ptr> parser2::ca_var_def_ops(parser_position start)
{
    (void)try_consume<u8'&'>();
    auto [error, var_name] = lex_variable_name(start);
    if (error)
        return {};
    std::vector<expressions::ca_expr_ptr> num;
    if (try_consume<u8'('>(hl_scopes::operator_symbol))
    {
        if (!is_num())
        {
            syntax_error_or_eof();
            return {};
        }
        auto [error_num, num_] = lex_num();
        if (error_num)
            return {};
        num.push_back(std::move(num_));
        if (!match<u8')'>(hl_scopes::operator_symbol, diagnostic_op::error_S0011))
            return {};
    }
    const auto r = range_from(start);
    semantics::vs_ptr var;
    if (std::holds_alternative<context::id_index>(var_name))
    {
        var = std::make_unique<semantics::basic_variable_symbol>(
            std::get<context::id_index>(var_name), std::move(num), r);
    }
    else
    {
        auto& cc = std::get<semantics::concat_chain>(var_name);
        var = std::make_unique<semantics::created_variable_symbol>(std::move(cc), std::move(num), r);
    }
    return { true, std::make_unique<semantics::var_ca_operand>(std::move(var), r) };
}

semantics::operand_list parser_holder::macro_ops(bool reparse)
{
    parser2 p(this);
    auto [ops, line_range] = p.macro_ops(reparse);

    if (!reparse)
    {
        collector.set_operand_remark_field(std::move(ops), std::move(p.remarks), line_range);
        return {};
    }
    else
        return std::move(ops);
}

void parser_holder::op_rem_body_ca_expr()
{
    parser2 p(this);

    auto [ops, line_range] = p.ca_args<&parser2::ca_expr_ops>();
    collector.set_operand_remark_field(std::move(ops), std::move(p.remarks), line_range);
}

void parser_holder::op_rem_body_ca_branch()
{
    parser2 p(this);

    auto [ops, line_range] = p.ca_args<&parser2::ca_branch_ops>();
    collector.set_operand_remark_field(std::move(ops), std::move(p.remarks), line_range);
}

void parser_holder::op_rem_body_ca_var_def()
{
    parser2 p(this);

    auto [ops, line_range] = p.ca_args<&parser2::ca_var_def_ops>();
    collector.set_operand_remark_field(std::move(ops), std::move(p.remarks), line_range);
}

semantics::operand_ptr parser_holder::ca_op_expr()
{
    parser2 p(this);
    const auto start = p.cur_pos_adjusted();
    auto [error, expr] = p.lex_expr_general();
    if (error || *p.input.next != EOF_SYMBOL)
        return nullptr;

    p.resolve_expression(expr);
    return std::make_unique<semantics::expr_ca_operand>(std::move(expr), p.range_from(start));
}

void parser2::lab_instr_process()
{
    assert(follows_PROCESS());

    const auto start = cur_pos_adjusted();
    for (size_t i = 0; i < PROCESS.size(); ++i)
        consume();

    const auto r = range_from(start);
    holder->collector.set_label_field(r);
    holder->collector.set_instruction_field(context::id_index("*PROCESS"), r);
    add_hl_symbol(r, hl_scopes::instruction);
}

parser_holder::op_data parser2::lab_instr_rest()
{
    if (eof())
    {
        const auto r = cur_pos_range();
        return parser_holder::op_data {
            .op_text = lexing::u8string_with_newlines(),
            .op_range = r,
            .op_logical_column = input.char_position_in_line,
        };
    }

    const auto op_start = cur_pos(); // intentionally not adjusted to capture newlines
    parser_holder::op_data result {
        .op_text = lexing::u8string_with_newlines(),
        .op_range = {},
        .op_logical_column = input.char_position_in_line,
    };

    result.op_text->text.reserve((input.last - input.next) + (&holder->newlines.back() - input.nl));
    while (!eof())
    {
        while (!before_nl())
        {
            result.op_text->text.push_back(lexing::u8string_view_with_newlines::EOL);
            ++input.line;
            ++input.nl;
            input.char_position_in_line = holder->cont;
            input.char_position_in_line_utf16 = holder->cont;
        }

        const auto ch = *input.next;

        do
        {
            result.op_text->text.push_back(*input.next);
            ++input.next;
        } while ((*input.next & 0xC0) == 0x80);

        ++input.char_position_in_line;
        input.char_position_in_line_utf16 += 1 + (ch >= first_long_utf16);
    }

    while (*input.nl != (size_t)-1)
    {
        result.op_text->text.push_back(lexing::u8string_view_with_newlines::EOL);
        ++input.line;
        ++input.nl;
        input.char_position_in_line = holder->cont;
        input.char_position_in_line_utf16 = holder->cont;
    }

    result.op_range = range_from(op_start);

    return result;
}

parser_holder::op_data parser2::lab_instr_empty(parser_position start)
{
    const auto r = empty_range(start);

    holder->collector.set_label_field(r);
    holder->collector.set_instruction_field(r);
    holder->collector.set_operand_remark_field(r);

    return {};
}

result_t<void> parser2::lex_label_string(concat_chain_builder& cb)
{
    assert(follows<u8'\''>());

    consume_into(cb.last_text_value());

    while (!eof())
    {
        switch (*input.next)
        {
            case u8'\'':
                consume_into(cb.last_text_value());
                return {};

            case u8'&':
                if (input.next[1] == u8'&')
                {
                    consume_into(cb.last_text_value());
                    consume_into(cb.last_text_value());
                }
                else
                {
                    cb.push_last_text();
                    auto [error, vs] = lex_variable();
                    if (error)
                        return failure;
                    cb.emplace_back(std::in_place_type<semantics::var_sym_conc>, std::move(vs));
                }
                break;

            default:
                consume_into(cb.last_text_value());
                break;
        }
    }
    add_diagnostic(diagnostic_op::error_S0005);
    return failure;
}

result_t<semantics::concat_chain> parser2::lex_label()
{
    semantics::concat_chain chain;
    concat_chain_builder cb(*this, chain, false);

    bool next_char_special = true;

    while (true)
    {
        const auto last_char_special = std::exchange(next_char_special, true);
        switch (*input.next)
        {
            case EOF_SYMBOL:
            case u8' ':
                cb.push_last_text();
                return chain;

            case u8'.':
                cb.push_dot();
                next_char_special = follows<u8'C', u8'c'>();
                break;

            case u8'=':
                cb.push_equals();
                next_char_special = follows<u8'C', u8'c'>();
                break;

            case u8'&':
                if (input.next[1] == u8'&')
                {
                    consume_into(cb.last_text_value());
                    consume_into(cb.last_text_value());
                }
                else
                {
                    cb.push_last_text();
                    auto [error, vs] = lex_variable();
                    if (error)
                        return failure;
                    cb.emplace_back(std::in_place_type<semantics::var_sym_conc>, std::move(vs));
                }
                break;

            case u8'\'':
                if (!last_char_special && input.next[1] == u8' ')
                    consume_into(cb.last_text_value());
                else if (auto [error] = lex_label_string(cb); error)
                    return failure;
                break;

            case u8'O':
            case u8'S':
            case u8'I':
            case u8'L':
            case u8'T':
            case u8'o':
            case u8's':
            case u8'i':
            case u8'l':
            case u8't':
                if (last_char_special && input.next[1] == u8'\'')
                {
                    consume_into(cb.last_text_value());
                    consume_into(cb.last_text_value());
                    break;
                }
                [[fallthrough]];

            case u8'C':
            case u8'c':
                if (last_char_special && input.next[1] == u8'\'')
                {
                    consume_into(cb.last_text_value());
                    if (auto [error] = lex_label_string(cb); error)
                        return failure;
                    break;
                }
                [[fallthrough]];

            default:
                next_char_special = !is_ord();

                consume_into(cb.last_text_value());

                break;
        }
    }
}

result_t<semantics::concat_chain> parser2::lex_instr()
{
    if (eof() || follows<u8' '>())
    {
        syntax_error_or_eof();
        return failure;
    }

    semantics::concat_chain result;
    concat_chain_builder cb(*this, result, false);

    while (true)
    {
        switch (*input.next)
        {
            case EOF_SYMBOL:
            case u8' ':
                cb.push_last_text();
                return result;

            case u8'\'':
                syntax_error_or_eof();
                return failure;

            case u8'=':
                cb.push_equals();
                break;

            case u8'.':
                cb.push_dot();
                break;

            case u8'&':
                if (input.next[1] == u8'&')
                {
                    consume_into(cb.last_text_value());
                    consume();
                }
                else
                {
                    cb.push_last_text();
                    auto [error, vs] = lex_variable();
                    if (error)
                        return failure;
                    cb.emplace_back(std::in_place_type<semantics::var_sym_conc>, std::move(vs));
                }
                break;

            default:
                consume_into(cb.last_text_value());
                break;
        }
    }
}

parser_holder::op_data parser2::lab_instr()
{
    if (eof())
        return lab_instr_empty(cur_pos());

    if (holder->process_allowed && follows_PROCESS())
    {
        lab_instr_process();
        return lab_instr_rest();
    }

    const auto start = cur_pos_adjusted();
    auto label_end = start;

    semantics::concat_chain label_concat;
    if (lex_optional_space())
    {
        if (eof())
            return lab_instr_empty(start);
    }
    else
    {
        auto [l_error, v] = lex_label();
        if (l_error)
            return {};

        label_end = cur_pos();
        label_concat = std::move(v);

        if (!lex_optional_space())
        {
            syntax_error_or_eof();
            return {};
        }
    }

    const auto instr_start = cur_pos_adjusted();
    auto [i_error, instr_concat] = lex_instr();
    if (i_error)
        return {};

    lex_handle_label(std::move(label_concat), remap_range(start, label_end));
    lex_handle_instruction(std::move(instr_concat), range_from(instr_start));

    return lab_instr_rest();
}

namespace {
constexpr bool is_ord_like(std::span<const semantics::concatenation_point> cc)
{
    if (std::ranges::any_of(
            cc, [](const auto& c) { return !std::holds_alternative<semantics::char_str_conc>(c.value); }))
        return false;
    const auto it = std::ranges::find_if(
        cc, [](const auto& c) { return !std::get<semantics::char_str_conc>(c.value).value.empty(); });
    if (it == cc.end())
        return false;
    if (!char_is_ord_first(std::get<semantics::char_str_conc>(it->value).value.front()))
        return false;
    return std::all_of(it, cc.end(), [](const auto& c) {
        const auto& str = std::get<semantics::char_str_conc>(c.value).value;
        return std::ranges::all_of(str, [](unsigned char uc) { return char_is_ord(uc); });
    });
}

constexpr bool has_variable_symbol(const semantics::concat_chain& cc) noexcept
{
    return std::ranges::any_of(
        cc, [](const auto& c) { return std::holds_alternative<semantics::var_sym_conc>(c.value); });
}

constexpr bool is_seq_symbol(const semantics::concat_chain& cc) noexcept
{
    return std::holds_alternative<semantics::dot_conc>(cc.front().value) && is_ord_like(std::span(cc).subspan(1));
}
} // namespace

void parser2::lex_handle_label(semantics::concat_chain cc, range r)
{
    if (cc.empty())
        holder->collector.set_label_field(r);
    else if (has_variable_symbol(cc))
    {
        semantics::concatenation_point::clear_concat_chain(cc);
        for (const auto& c : cc)
        {
            if (!std::holds_alternative<semantics::char_str_conc>(c.value))
                continue;
            add_hl_symbol(std::get<semantics::char_str_conc>(c.value).conc_range, hl_scopes::label);
        }
        holder->collector.set_label_field(std::move(cc), r);
    }
    else if (is_seq_symbol(cc)) // seq symbol
    {
        auto& label = std::get<semantics::char_str_conc>(cc[1].value).value;
        for (auto& c : std::span(cc).subspan(2))
            label.append(std::get<semantics::char_str_conc>(c.value).value);

        add_hl_symbol(r, hl_scopes::seq_symbol);
        holder->collector.set_label_field({ parse_identifier(std::move(label), r), r }, r);
    }
    else if (is_ord_like(cc))
    {
        std::string label = semantics::concatenation_point::to_string(std::move(cc));
        add_hl_symbol(r, hl_scopes::label);
        auto id = add_id(label);
        holder->collector.set_label_field({ id, std::move(label) }, r);
    }
    else
    {
        add_hl_symbol(r, hl_scopes::label);
        holder->collector.set_label_field(semantics::concatenation_point::to_string(std::move(cc)), r);
    }
}

void parser2::lex_handle_instruction(semantics::concat_chain cc, range r)
{
    assert(!cc.empty());

    if (std::ranges::any_of(cc, [](const auto& c) { return std::holds_alternative<semantics::var_sym_conc>(c.value); }))
    {
        for (const auto& point : cc)
        {
            if (!std::holds_alternative<semantics::char_str_conc>(point.value))
                continue;
            add_hl_symbol(std::get<semantics::char_str_conc>(point.value).conc_range, hl_scopes::instruction);
        }

        holder->collector.set_instruction_field(std::move(cc), r);
    }
    else if (is_ord_like(std::span(cc).first(1)))
    {
        add_hl_symbol(r, hl_scopes::instruction);
        auto instr_id = parse_identifier(semantics::concatenation_point::to_string(std::move(cc)), r);
        holder->collector.set_instruction_field(instr_id, r);
    }
    else
    {
        add_hl_symbol(r, hl_scopes::instruction);
        auto instr_id = add_id(semantics::concatenation_point::to_string(std::move(cc)));
        holder->collector.set_instruction_field(instr_id, r);
    }
}

parser_holder::op_data parser2::look_lab_instr_seq()
{
    const auto start = cur_pos_adjusted();
    consume();
    if (!is_ord_first())
        return lab_instr_empty(start);

    std::string label = lex_ord();

    const auto seq_end = cur_pos();

    const auto label_r = remap_range(start, seq_end);
    auto seq_symbol = semantics::seq_sym { parse_identifier(std::move(label), label_r), label_r };
    holder->collector.set_label_field(seq_symbol, label_r);

    if (!lex_optional_space() || !is_ord_first())
    {
        const auto r = empty_range(seq_end);
        holder->collector.set_instruction_field(r);
        holder->collector.set_operand_remark_field(r);
        return {};
    }
    const auto instr_start = cur_pos_adjusted();
    std::string instr = lex_ord();
    const auto instr_end = cur_pos();

    if (!eof() && !follows<u8' '>())
    {
        const auto r = empty_range(seq_end);
        holder->collector.set_instruction_field(r);
        holder->collector.set_operand_remark_field(r);
        return {};
    }

    const auto instr_r = remap_range(instr_start, instr_end);

    holder->collector.set_instruction_field(parse_identifier(std::move(instr), instr_r), instr_r);

    auto result = lab_instr_rest();

    holder->collector.set_operand_remark_field(result.op_range);

    return result;
}

parser_holder::op_data parser2::look_lab_instr()
{
    const auto start = cur_pos_adjusted();

    std::string label;
    range label_r = empty_range(start);
    switch (*input.next)
    {
        case EOF_SYMBOL:
            return lab_instr_empty(start);
        case u8'.':
            return look_lab_instr_seq();

        default:
            if (!is_ord_first())
                return lab_instr_empty(start);
            label = lex_ord();
            label_r = range_from(start);
            break;

        case u8' ':
            break;
    }

    if (!lex_optional_space())
        return lab_instr_empty(start);
    if (!is_ord_first())
        return lab_instr_empty(start);

    const auto instr_start = cur_pos_adjusted();
    auto instr = lex_ord();
    const auto instr_r = range_from(instr_start);

    if (!eof() && !follows<u8' '>())
        return lab_instr_empty(start);

    if (!label.empty())
    {
        const auto id = add_id(label);
        holder->collector.set_label_field(semantics::ord_symbol_string { id, std::move(label) }, label_r);
    }
    holder->collector.set_instruction_field(parse_identifier(std::move(instr), instr_r), instr_r);

    auto result = lab_instr_rest();

    holder->collector.set_operand_remark_field(result.op_range);

    return result;
}

parser_holder::op_data parser_holder::lab_instr()
{
    parser2 p(this);

    // TODO: diagnose instruction not finished on the initial line
    // const auto initial_state = p.input;

    return p.lab_instr();
}

parser_holder::op_data parser_holder::look_lab_instr()
{
    parser2 p(this);

    return p.look_lab_instr();
}

result_t<void> parser2::lex_deferred_string(std::vector<semantics::vs_ptr>& vs)
{
    disable_ca_string();
    utils::scope_exit e([this]() noexcept { enable_ca_string(); });

    const auto string_start = cur_pos_adjusted();
    consume();

    while (true)
    {
        switch (*input.next)
        {
            case EOF_SYMBOL:
                syntax_error_or_eof();
                add_hl_symbol(range_from(string_start), hl_scopes::string);
                return failure;

            case u8'\'':
                if (input.next[1] != u8'\'')
                {
                    consume();

                    add_hl_symbol(range_from(string_start), hl_scopes::string);
                    return {};
                }
                consume();
                consume();
                break;

            case u8'&':
                if (input.next[1] == u8'&')
                {
                    consume();
                    consume();
                }
                else if (auto [error, v] = lex_variable(); error)
                    return failure;
                else
                    vs.push_back(std::move(v));
                break;

            default:
                consume();
                break;
        }
    }
}

void parser2::op_rem_body_deferred()
{
    const auto start = cur_pos_adjusted();
    if (eof())
    {
        holder->collector.set_operand_remark_field(empty_range(start));
        return;
    }
    if (!follows<u8' '>())
    {
        syntax_error_or_eof();
        return;
    }
    consume_spaces();

    auto rest = parser2(*this).lab_instr_rest();

    std::vector<semantics::vs_ptr> vs;

    bool next_char_special = true;

    while (!eof())
    {
        const auto last_char_special = std::exchange(next_char_special, true);
        switch (*input.next)
        {
            case u8',':
                consume(hl_scopes::operator_symbol);
                process_optional_line_remark();
                break;

            case u8' ':
                lex_last_remark();
                break;

            case u8'*':
            case u8'/':
            case u8'+':
            case u8'-':
            case u8'=':
            case u8'.':
            case u8'(':
            case u8')':
                consume(hl_scopes::operator_symbol);
                break;

            case u8'\'':
                if (auto [error] = lex_deferred_string(vs); error)
                    return;
                break;

            case u8'&':
                switch (input.next[1])
                {
                    case EOF_SYMBOL:
                        consume();
                        add_diagnostic(diagnostic_op::error_S0003);
                        return;

                    case u8'&':
                        consume();
                        consume();
                        break;

                    default:
                        if (auto [error, v] = lex_variable(); error)
                            return;
                        else
                            vs.push_back(std::move(v));
                        break;
                }
                break;

            case u8'O':
            case u8'S':
            case u8'I':
            case u8'L':
            case u8'T':
            case u8'o':
            case u8's':
            case u8'i':
            case u8'l':
            case u8't':
                if (last_char_special && follows<mach_attrs, group<u8'\''>, attr_argument>())
                {
                    const auto p = cur_pos_adjusted();
                    consume();
                    consume();
                    add_hl_symbol(range_from(p), hl_scopes::data_attr_type);
                    next_char_special = false;
                    break;
                }
                [[fallthrough]];
            default: {
                const auto substart = cur_pos_adjusted();
                while (except<u8'&', u8' ', u8',', u8'*', u8'/', u8'+', u8'-', u8'=', u8'.', u8'(', u8')', u8'\''>())
                {
                    next_char_special = !is_ord();
                    consume();
                    if (next_char_special)
                        break;
                }
                add_hl_symbol(range_from(substart), hl_scopes::operand);
            }
        }
    }
    holder->collector.set_operand_remark_field(
        std::move(*rest.op_text), std::move(vs), std::move(remarks), rest.op_range, rest.op_logical_column);
}

void parser_holder::op_rem_body_deferred()
{
    parser2 p(this);

    p.op_rem_body_deferred();
}

void parser2::op_rem_body_noop()
{
    (void)lex_optional_space();

    if (eof())
    {
        const auto r = cur_pos_range();
        holder->collector.set_operand_remark_field(semantics::operand_list(), semantics::remark_list(), r);
    }
    else
    {
        const auto start = cur_pos_adjusted();
        while (!eof())
            consume();

        const auto r = range_from(start);
        holder->collector.set_operand_remark_field(semantics::operand_list {}, semantics::remark_list { r }, r);
    }
}

void parser_holder::op_rem_body_noop()
{
    parser2 p(this);

    p.op_rem_body_noop();
}

result_t<void> parser2::lex_rest_of_model_string(concat_chain_builder& ccb)
{
    while (true)
    {
        switch (*input.next)
        {
            case EOF_SYMBOL:
                return failure;

            case u8'&':
                if (input.next[1] == u8'&')
                {
                    consume_into(ccb.last_text_value());
                    consume_into(ccb.last_text_value());
                    break;
                }
                ccb.push_last_text();
                if (auto [error, vs] = lex_variable(); error)
                    return failure;
                else
                    ccb.emplace_back(std::in_place_type<semantics::var_sym_conc>, std::move(vs));
                break;

            case u8'.':
                ccb.push_dot();
                break;

            case u8'=':
                ccb.push_equals();
                break;

            case u8'\'':
                consume_into(ccb.last_text_value());
                return {};

            default:
                consume_into(ccb.last_text_value());
                break;
        }
    }
}

result_t<parser2::before_model> parser2::model_before_variable()
{
    bool next_char_special = true;
    std::optional<parser_position> in_string;
    while (true)
    {
        const bool last_char_special = std::exchange(next_char_special, true);
        switch (*input.next)
        {
            case EOF_SYMBOL:
            case u8' ':
                return { false, next_char_special, in_string };

            case u8'&':
                if (input.next[1] == u8'&')
                {
                    consume();
                    consume();
                    break;
                }
                else
                    return { true, next_char_special, in_string };

            case u8'\'': {
                in_string = cur_pos_adjusted();
                consume();
                while (except<u8'\''>())
                {
                    if (follows<group<u8'&'>, group<u8'&'>>())
                        consume();
                    else if (follows<u8'&'>())
                        return { true, next_char_special, in_string };
                    consume();
                }
                if (!match<u8'\''>(diagnostic_op::error_S0005))
                    return failure;
                add_hl_symbol(range_from(*in_string), hl_scopes::string);
                in_string.reset();
                break;
            }

            case u8',':
            case u8'(':
            case u8')':
                consume(hl_scopes::operator_symbol);
                break;

            case u8'=':
                consume();
                next_char_special = false;
                break;

            case u8'O':
            case u8'S':
            case u8'I':
            case u8'L':
            case u8'T':
            case u8'o':
            case u8's':
            case u8'i':
            case u8'l':
            case u8't':
                consume();
                if (last_char_special && follows<group<u8'\''>, attr_argument>())
                    consume();
                next_char_special = false;
                break;

            default:
                next_char_special = !is_ord();
                consume();
                break;
        }
    }
}

result_t<std::optional<semantics::op_rem>> parser2::try_model_ops(parser_position line_start)
{
    const auto start = cur_pos_adjusted();
    const auto initial = input.next;

    semantics::concat_chain cc;
    concat_chain_builder ccb(*this, cc);

    auto [before_error, before_res] = model_before_variable();
    if (before_error)
        return failure;
    auto& [variable_follows, next_char_special, in_string] = before_res;
    if (!variable_follows)
        return std::nullopt;

    assert(follows<u8'&'>());

    if (initial != input.next)
        cc.emplace_back(std::in_place_type<semantics::char_str_conc>, capture_text(initial), range_from(start));

    ccb.push_last_text();
    if (auto [error, vs] = lex_variable(); error)
        return failure;
    else
        ccb.emplace_back(std::in_place_type<semantics::var_sym_conc>, std::move(vs));

    if (in_string)
    {
        auto [error] = lex_rest_of_model_string(ccb);
        add_hl_symbol(range_from(*in_string), hl_scopes::string);
        if (error)
            return failure;
        in_string.reset();
    }

    std::optional<parser_position> operand_end;
    while (true)
    {
        const bool last_char_special = std::exchange(next_char_special, true);
        switch (*input.next)
        {
            case u8' ':
                operand_end.emplace(cur_pos());
                lex_last_remark();
                [[fallthrough]];

            case EOF_SYMBOL: {
                if (!operand_end)
                    operand_end.emplace(cur_pos());
                semantics::op_rem result {
                    .operands = {},
                    .remarks = std::move(remarks),
                    .line_range = range_from(line_start),
                };
                semantics::concatenation_point::clear_concat_chain(cc);
                resolve_concat_chain(cc);
                result.operands.emplace_back(std::make_unique<semantics::model_operand>(
                    std::move(cc), holder->line_limits, remap_range(start, *operand_end)));
                return result;
            }

            case u8'&':
                if (input.next[1] == u8'&')
                {
                    consume_into(ccb.last_text_value());
                    consume_into(ccb.last_text_value());
                }
                else if (auto [error, vs] = (ccb.push_last_text(), lex_variable()); error)
                    return failure;
                else
                    ccb.emplace_back(std::in_place_type<semantics::var_sym_conc>, std::move(vs));
                break;

            case u8'\'': {
                const auto string_start = cur_pos_adjusted();
                consume_into(ccb.last_text_value());
                auto [error] = lex_rest_of_model_string(ccb);
                add_hl_symbol(range_from(string_start), hl_scopes::string);
                if (error)
                    return failure;
                break;
            }

            case u8'(':
            case u8')':
            case u8',':
                consume_into(ccb.last_text_value(), hl_scopes::operator_symbol);
                break;


            case u8'.':
                ccb.push_dot();
                break;

            case u8'=':
                ccb.push_equals();
                next_char_special = false;
                break;

            case u8'O':
            case u8'S':
            case u8'I':
            case u8'L':
            case u8'T':
            case u8'o':
            case u8's':
            case u8'i':
            case u8'l':
            case u8't':
                consume_into(ccb.last_text_value());
                if (last_char_special && follows<group<u8'\''>, attr_argument>())
                    consume_into(ccb.last_text_value());
                next_char_special = false;
                break;

            default:
                next_char_special = !is_ord();
                consume_into(ccb.last_text_value());
                break;
        }
    }
}

result_t<semantics::operand_ptr> parser2::mach_op()
{
    const auto start = cur_pos_adjusted();
    auto [disp_error, disp] = lex_mach_expr();
    if (disp_error)
        return failure;

    if (!try_consume<u8'('>(hl_scopes::operator_symbol))
        return std::make_unique<semantics::expr_machine_operand>(std::move(disp), range_from(start));

    expressions::mach_expr_ptr e1, e2;
    if (eof())
    {
        syntax_error_or_eof();
        return failure;
    }

    if (except<u8','>())
    {
        if (auto [error, e] = lex_mach_expr(); error)
            return failure;
        else
            e1 = std::move(e);
    }
    bool parsed_comma = false;
    if ((parsed_comma = try_consume<u8','>(hl_scopes::operator_symbol)) && !follows<u8')'>())
    {
        if (auto [error, e] = lex_mach_expr(); error)
            return failure;
        else
            e2 = std::move(e);
    }

    if (!match<u8')'>(hl_scopes::operator_symbol, diagnostic_op::error_S0011))
        return failure;

    if (e1 && e2)
        return std::make_unique<semantics::address_machine_operand>(
            std::move(disp), std::move(e1), std::move(e2), range_from(start), checking::operand_state::PRESENT);
    if (e2)
        return std::make_unique<semantics::address_machine_operand>(
            std::move(disp), nullptr, std::move(e2), range_from(start), checking::operand_state::FIRST_OMITTED);

    if (e1 && !parsed_comma)
        return std::make_unique<semantics::address_machine_operand>(
            std::move(disp), nullptr, std::move(e1), range_from(start), checking::operand_state::ONE_OP);

    return std::make_unique<semantics::address_machine_operand>(
        std::move(disp), std::move(e1), nullptr, range_from(start), checking::operand_state::SECOND_OMITTED);
}

result_t<semantics::operand_ptr> parser2::dat_op()
{
    const auto start = cur_pos_adjusted();
    const auto disabled = disable_literals();

    auto [error, d] = lex_data_definition(false);
    if (error)
        return failure;
    return std::make_unique<semantics::data_def_operand_inline>(std::move(d), range_from(start));
}
template<result_t<semantics::operand_ptr> (parser2::*first)(), result_t<semantics::operand_ptr> (parser2::*rest)()>
std::optional<semantics::op_rem> parser2::with_model(bool reparse, bool model_allowed) requires(first != nullptr)
{
    const auto start = cur_pos(); // capture true beginning
    if (eof())
        return semantics::op_rem { .line_range = empty_range(start) };

    if (auto [error] = handle_initial_space(reparse); error)
        return std::nullopt;

    if (eof())
        return semantics::op_rem { .line_range = empty_range(start) };

    if (model_allowed && std::find(input.next, input.last, u8'&') != input.last)
    {
        auto model_parser = *this;
        if (auto [error, result] = model_parser.try_model_ops(start); error)
            return std::nullopt;
        else if (result)
            return std::move(*result);
        holder->collector.prepare_for_next_statement();
    }

    std::vector<semantics::operand_ptr> operands;

    bool has_error = false;
    if (follows<u8','>())
        operands.push_back(std::make_unique<semantics::empty_operand>(cur_pos_range()));
    else if (auto [error, op] = (this->*first)(); (has_error = error))
        operands.push_back(std::make_unique<semantics::empty_operand>(cur_pos_range()));
    else
        operands.push_back(std::move(op));

    if (!has_error && ((rest == nullptr && except<u8' '>()) || except<u8',', u8' '>()))
    {
        has_error = true;
        syntax_error_or_eof();
    }

    while (!has_error && follows<u8','>())
    {
        consume(hl_scopes::operator_symbol);
        if (follows<u8','>())
            operands.push_back(std::make_unique<semantics::empty_operand>(cur_pos_range()));
        else if (eof() || follows<u8' '>())
        {
            operands.push_back(std::make_unique<semantics::empty_operand>(cur_pos_range()));
            break;
        }
        else if (auto [error_inner, op_inner] = (this->*rest)(); error_inner)
        {
            operands.push_back(std::make_unique<semantics::empty_operand>(cur_pos_range()));
            break;
        }
        else
        {
            operands.push_back(std::move(op_inner));
            if (except<u8',', u8' '>())
            {
                syntax_error_or_eof();
                break;
            }
        }
    }

    consume_rest();

    return semantics::op_rem {
        .operands = std::move(operands),
        .remarks = std::move(remarks),
        .line_range = range_from(start),
    };
}
std::optional<semantics::op_rem> parser_holder::op_rem_body_mach(bool reparse, bool model_allowed)
{
    parser2 p(this);

    return p.with_model<&parser2::mach_op>(reparse, model_allowed);
}

std::optional<semantics::op_rem> parser_holder::op_rem_body_dat(bool reparse, bool model_allowed)
{
    parser2 p(this);

    return p.with_model<&parser2::dat_op>(reparse, model_allowed);
}

result_t<semantics::operand_ptr> parser2::alias_op()
{
    const auto start = cur_pos_adjusted();
    const auto initial = input.next;

    if (!match<u8'C', u8'c', u8'X', u8'x'>(hl_scopes::self_def_type))
        return failure;

    if (!must_follow<u8'\''>())
        return failure;

    auto [error, s] = lex_simple_string();
    if (error)
        return failure;

    const auto r = range_from(start);
    add_hl_symbol(r, hl_scopes::self_def_type);

    return std::make_unique<semantics::expr_assembler_operand>(
        std::make_unique<expressions::mach_expr_default>(r), capture_text(initial), r);
}

result_t<semantics::operand_ptr> parser2::end_op()
{
    const auto start = cur_pos_adjusted();
    if (!match<u8'('>(hl_scopes::operator_symbol))
        return failure;

    std::string s1, s2, s3;

    auto op_start = cur_pos_adjusted();
    while (except<u8','>())
        consume_into(s1);
    const auto r1 = range_from(op_start);

    if (!match<u8','>(hl_scopes::operator_symbol))
        return failure;

    op_start = cur_pos_adjusted();
    while (except<u8','>())
        consume_into(s2);
    const auto r2 = range_from(op_start);

    if (!match<u8','>(hl_scopes::operator_symbol))
        return failure;

    op_start = cur_pos_adjusted();
    while (except<u8')'>())
        consume_into(s3);
    const auto r3 = range_from(op_start);

    if (!match<u8')'>(hl_scopes::operator_symbol))
        return failure;

    std::vector<std::unique_ptr<semantics::complex_assembler_operand::component_value_t>> language_triplet;
    language_triplet.push_back(
        std::make_unique<semantics::complex_assembler_operand::string_value_t>(std::move(s1), r1));
    language_triplet.push_back(
        std::make_unique<semantics::complex_assembler_operand::string_value_t>(std::move(s2), r2));
    language_triplet.push_back(
        std::make_unique<semantics::complex_assembler_operand::string_value_t>(std::move(s3), r3));
    add_hl_symbol(r1, hl_scopes::operand);
    add_hl_symbol(r2, hl_scopes::operand);
    add_hl_symbol(r3, hl_scopes::operand);

    const auto r = range_from(start);
    return std::make_unique<semantics::complex_assembler_operand>("", std::move(language_triplet), r);
}

result_t<semantics::operand_ptr> parser2::using_op1()
{
    const auto start = cur_pos_adjusted();
    if (!try_consume<u8'('>(hl_scopes::operator_symbol))
        return asm_mach_expr();

    const auto initial1 = input.next;
    auto [error1, e1] = lex_mach_expr();
    if (error1)
        return failure;
    const auto end1 = input.next;

    if (!try_consume<u8','>(hl_scopes::operator_symbol))
    {
        if (!match<u8')'>(hl_scopes::operator_symbol))
            return failure;

        const auto r = range_from(start);
        return std::make_unique<semantics::expr_assembler_operand>(
            std::move(e1), utils::to_upper_copy(capture_text(initial1, end1)), r);
    }

    const auto initial2 = input.next;
    auto [error2, e2] = lex_mach_expr();
    if (error2)
        return failure;
    const auto end2 = input.next;

    if (!match<u8')'>(hl_scopes::operator_symbol))
        return failure;

    const auto r = range_from(start);
    return std::make_unique<semantics::using_instr_assembler_operand>(
        std::move(e1), std::move(e2), capture_text(initial1, end1), capture_text(initial2, end2), r);
}

result_t<semantics::operand_ptr> parser2::asm_mach_expr()
{
    const auto start = cur_pos_adjusted();
    const auto initial = input.next;

    auto [error, expr] = lex_mach_expr();
    if (error)
        return failure;

    const auto r = range_from(start);
    return std::make_unique<semantics::expr_assembler_operand>(
        std::move(expr), utils::to_upper_copy(capture_text(initial)), r);
}

constexpr bool parser2::ord_followed_by_parenthesis() const noexcept
{
    if (!is_ord_first())
        return false;

    const auto* p = input.next;

    while (char_is_ord(*p))
        ++p;

    return *p == u8'(';
}

result_t<std::unique_ptr<semantics::complex_assembler_operand::component_value_t>> parser2::asm_op_inner()
{
    const auto start = cur_pos_adjusted();
    if (follows<u8'\''>())
    {
        auto [error, s] = lex_simple_string();
        if (error)
            return failure;
        const auto r = range_from(start);
        return std::make_unique<semantics::complex_assembler_operand::string_value_t>(std::move(s), r);
    }
    if (is_num())
    {
        const auto [error, v] = lex_number_as_int();
        if (error)
            return failure;

        const auto& [n, r] = v;
        add_hl_symbol(r, hl_scopes::operand);
        return std::make_unique<semantics::complex_assembler_operand::int_value_t>(n, r);
    }

    if (follows<u8',', u8')'>())
        return std::make_unique<semantics::complex_assembler_operand::string_value_t>("", empty_range(start));

    if (!is_ord_first())
    {
        syntax_error_or_eof();
        return failure;
    }

    auto id = lex_ord_upper();

    add_hl_symbol(range_from(start), hl_scopes::operand);

    if (!try_consume<u8'('>(hl_scopes::operator_symbol))
        return std::make_unique<semantics::complex_assembler_operand::string_value_t>(std::move(id), range_from(start));

    auto [error, nested] = asm_op_comma_c();
    if (error)
        return failure;

    if (!match<u8')'>(hl_scopes::operator_symbol))
        return failure;

    const auto r = range_from(start);
    return std::make_unique<semantics::complex_assembler_operand::composite_value_t>(
        std::move(id), std::move(nested), r);
}
result_t<std::vector<std::unique_ptr<semantics::complex_assembler_operand::component_value_t>>>
parser2::asm_op_comma_c()
{
    std::vector<std::unique_ptr<semantics::complex_assembler_operand::component_value_t>> result;
    if (auto [error, op] = asm_op_inner(); error)
        return failure;
    else
        result.push_back(std::move(op));

    while (try_consume<u8','>(hl_scopes::operator_symbol))
    {
        if (auto [error, op] = asm_op_inner(); error)
            return failure;
        else
            result.push_back(std::move(op));
    }

    return result;
}


result_t<semantics::operand_ptr> parser2::asm_op()
{
    const auto start = cur_pos_adjusted();
    if (follows<u8'\''>())
    {
        auto [error, s] = lex_simple_string();
        if (error)
            return failure;
        return std::make_unique<semantics::string_assembler_operand>(std::move(s), range_from(start));
    }

    if (!ord_followed_by_parenthesis())
        return asm_mach_expr();

    auto id = lex_ord_upper();
    add_hl_symbol(range_from(start), hl_scopes::operand);

    if (!match<u8'('>(hl_scopes::operator_symbol))
        return failure;

    auto [error, nested] = asm_op_comma_c();
    if (error)
        return failure;

    if (!match<u8')'>(hl_scopes::operator_symbol))
        return failure;

    const auto r = range_from(start);
    return std::make_unique<semantics::complex_assembler_operand>(std::move(id), std::move(nested), r);
}

std::optional<semantics::op_rem> parser_holder::op_rem_body_asm(
    context::id_index opcode, bool reparse, bool model_allowed)
{
    parser2 p(this);

    static constexpr context::id_index ALIAS("ALIAS");
    static constexpr context::id_index USING("USING");
    static constexpr context::id_index END("END");

    if (opcode == ALIAS)
        return p.with_model<&parser2::alias_op, nullptr>(reparse, model_allowed);
    else if (opcode == USING)
        return p.with_model<&parser2::using_op1, &parser2::asm_mach_expr>(reparse, model_allowed);
    else if (opcode == END)
        return p.with_model<&parser2::asm_op, &parser2::end_op>(reparse, model_allowed);
    else
        return p.with_model<&parser2::asm_op>(reparse, model_allowed);
}

semantics::operand_ptr parser_holder::operand_mach()
{
    parser2 p(this);
    auto [error, op] = p.mach_op();
    if (error || *p.input.next != EOF_SYMBOL)
        return nullptr;

    return std::move(op);
}

void parser2::lookahead_operands_and_remarks_dat()
{
    const auto start = cur_pos_adjusted();
    if (eof() || !lex_optional_space() || eof())
    {
        holder->collector.set_operand_remark_field({}, {}, empty_range(start));
        return;
    }
    auto [error, op] = dat_op();
    if (error)
    {
        holder->collector.set_operand_remark_field({}, {}, empty_range(start));
        return;
    }
    semantics::operand_list operands;
    operands.push_back(std::move(op));
    range r = range_from(start);
    holder->collector.set_operand_remark_field(std::move(operands), std::vector<range>(), r);
}

void parser_holder::lookahead_operands_and_remarks_dat()
{
    parser2 p(this);

    p.lookahead_operands_and_remarks_dat();
}

void parser2::lookahead_operands_and_remarks_asm()
{
    auto start = cur_pos_adjusted();
    if (eof() || !lex_optional_space() || eof())
    {
        holder->collector.set_operand_remark_field({}, {}, empty_range(start));
        return;
    }
    semantics::operand_list operands;

    bool errors = false;
    auto op_start = cur_pos_adjusted();

    if (auto [error, op] = asm_op(); error)
    {
        errors = true;
        operands.push_back(std::make_unique<semantics::empty_operand>(empty_range(op_start)));
        while (except<u8',', u8' '>())
            consume();
    }
    else
        operands.push_back(std::move(op));

    while (follows<u8','>())
    {
        consume();
        op_start = cur_pos_adjusted();
        if (follows<u8',', u8' '>())
        {
            operands.push_back(std::make_unique<semantics::empty_operand>(empty_range(op_start)));
            continue;
        }
        auto [error, op] = asm_op();
        if (error || errors)
        {
            errors = true;
            operands.push_back(std::make_unique<semantics::empty_operand>(empty_range(op_start)));
            while (except<u8',', u8' '>())
                consume();
        }
        else
            operands.push_back(std::move(op));
    }
    const range r = range_from(start);
    holder->collector.set_operand_remark_field(std::move(operands), std::vector<range>(), r);
}

void parser_holder::lookahead_operands_and_remarks_asm()
{
    parser2 p(this);

    p.lookahead_operands_and_remarks_asm();
}

semantics::literal_si parser_holder::literal_reparse()
{
    parser2 p(this);

    auto [error, lit] = p.lex_literal();
    if (error || !p.eof())
        return nullptr;
    return std::move(lit);
}

expressions::ca_expr_ptr parser_holder::testing_expr()
{
    parser2 p(this);
    auto [error, e] = p.lex_expr();
    if (error)
    {
        p.add_diagnostic(diagnostic_op::error_S0002);
        return {};
    }
    else
        return std::move(e);
}

expressions::data_definition parser_holder::testing_data_def()
{
    parser2 p(this);
    auto [error, dd] = p.lex_data_definition(false);
    if (error)
    {
        p.add_diagnostic(diagnostic_op::error_S0002);
        return {};
    }
    else
        return std::move(dd);
}

} // namespace hlasm_plugin::parser_library::parsing
