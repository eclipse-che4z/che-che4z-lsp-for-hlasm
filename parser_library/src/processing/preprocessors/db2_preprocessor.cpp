/*
 * Copyright (c) 2021 Broadcom.
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

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <concepts>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <regex>
#include <stack>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "diagnostic_consumer.h"
#include "document.h"
#include "lexing/logical_line.h"
#include "parse_lib_provider.h"
#include "preprocessor_options.h"
#include "preprocessor_utils.h"
#include "processing/preprocessor.h"
#include "range.h"
#include "semantics/range_provider.h"
#include "semantics/source_info_processor.h"
#include "semantics/statement.h"
#include "utils/concat.h"
#include "utils/resource_location.h"
#include "utils/string_operations.h"
#include "utils/task.h"
#include "utils/text_matchers.h"
#include "utils/unicode_text.h"

namespace hlasm_plugin::parser_library::processing {
namespace {
using utils::concat;

enum class symbol_type : unsigned char
{
    other_char,
    ord_char,
    blank,
    colon,
    quote,
    remark_start,
};

constexpr std::array symbols = []() {
    std::array<symbol_type, std::numeric_limits<unsigned char>::max() + 1> r {};

    using enum symbol_type;

    for (unsigned char c = '0'; c <= '9'; ++c)
        r[c] = ord_char;
    for (unsigned char c = 'A'; c <= 'Z'; ++c)
        r[c] = ord_char;
    for (unsigned char c = 'a'; c <= 'z'; ++c)
        r[c] = ord_char;

    r[(unsigned char)'_'] = ord_char;
    r[(unsigned char)'@'] = ord_char;
    r[(unsigned char)'$'] = ord_char;
    r[(unsigned char)'#'] = ord_char;
    r[(unsigned char)' '] = blank;
    r[(unsigned char)':'] = colon;
    r[(unsigned char)'\''] = quote;
    r[(unsigned char)'\"'] = quote;
    r[(unsigned char)'-'] = remark_start;

    return r;
}();

class db2_logical_line_helper
{
public:
    lexing::logical_line<std::string_view::iterator> m_orig_ll;
    lexing::logical_line<std::string_view::iterator> m_db2_ll;
    size_t m_lineno = 0;
    std::vector<std::optional<std::string_view>> m_comments;

    db2_logical_line_helper() = default;

    preprocessor::line_iterator reinit(preprocessor::line_iterator it, preprocessor::line_iterator end, size_t lineno)
    {
        m_lineno = lineno;

        it = preprocessor::extract_nonempty_logical_line(m_orig_ll, it, end, lexing::default_ictl);
        m_db2_ll = m_orig_ll;
        extract_db2_line_comments(m_db2_ll, m_comments);

        return it;
    }

    template<std::forward_iterator It>
    static void trim_left(It& it, const It& it_e)
    {
        while (it != it_e)
        {
            if (*it == ' ')
                it = std::next(it);
            else if (auto it_n = std::next(it); *it == '-' && (it_n != it_e && *it_n == '-'))
                it = std::next(it_n);
            else
                break;
        }
    }

private:
    template<typename It>
    It find_start_of_line_comment(
        std::stack<unsigned char, std::basic_string<unsigned char>>& quotes, It code, const It& code_end) const
    {
        bool comment_possibly_started = false;
        for (; code != code_end; ++code)
        {
            const unsigned char c = *code;
            if (auto s = symbols[c]; s == symbol_type::quote)
            {
                if (quotes.empty() || quotes.top() != c)
                    quotes.push(c);
                else if (quotes.top() == c)
                    quotes.pop();

                comment_possibly_started = false;
            }
            else if (quotes.empty() && s == symbol_type::remark_start)
            {
                if (!comment_possibly_started)
                    comment_possibly_started = true;
                else
                    break;
            }
        }

        return code;
    }

    void extract_db2_line_comments(lexing::logical_line<std::string_view::iterator>& ll,
        std::vector<std::optional<std::string_view>>& comments) const
    {
        comments.clear();
        std::stack<unsigned char, std::basic_string<unsigned char>> quotes;
        for (auto& seg : ll.segments)
        {
            auto& comment = comments.emplace_back(std::nullopt);

            // code part will contain the '--' separator if comment is detected
            if (auto comment_start = find_start_of_line_comment(quotes, seg.code, seg.continuation);
                comment_start != seg.continuation)
            {
                std::advance(comment_start, 1);
                comment = std::string_view(comment_start, seg.continuation);
                seg.continuation = comment_start;
                seg.ignore = comment_start;
            }
        }
    }
};

template<typename It>
class mini_parser
{
    void skip_to_matching_character(It& b, const It& e) const
    {
        if (b == e)
            return;

        const auto to_match = *b;

        while (++b != e && to_match != *b)
            ;
    }

public:
    std::vector<semantics::preproc_details::name_range> get_args(It b, const It& e, size_t lineno) const
    {
        enum class consuming_state
        {
            NON_CONSUMING,
            PREPARE_TO_CONSUME,
            CONSUMING,
            TRAIL,
            QUOTE,
        };

        std::vector<semantics::preproc_details::name_range> arguments;
        const auto try_arg_inserter = [&arguments, &lineno](const It& start, const It& end, consuming_state state) {
            if (state != consuming_state::CONSUMING)
                return false;

            arguments.emplace_back(semantics::preproc_details::name_range {
                std::string(start, end), semantics::text_range(start, end, lineno) });
            return true;
        };

        It arg_start_it = b;
        consuming_state next_state = consuming_state::NON_CONSUMING;
        while (b != e)
        {
            const auto state = std::exchange(next_state, consuming_state::NON_CONSUMING);

            switch (symbols[static_cast<unsigned char>(*b)])
            {
                using enum symbol_type;
                case ord_char:
                    if (state == consuming_state::PREPARE_TO_CONSUME)
                    {
                        arg_start_it = b;
                        next_state = consuming_state::CONSUMING;
                    }
                    else if (state == consuming_state::CONSUMING)
                        next_state = state;

                    break;

                case colon:
                    if (state == consuming_state::PREPARE_TO_CONSUME || state == consuming_state::TRAIL)
                        break;

                    try_arg_inserter(arg_start_it, b, state);
                    next_state = consuming_state::PREPARE_TO_CONSUME;

                    break;

                case blank:
                    if (try_arg_inserter(arg_start_it, b, state))
                        next_state = consuming_state::TRAIL;
                    else
                        next_state = state;

                    break;

                case quote:
                    try_arg_inserter(arg_start_it, b, state);

                    if (skip_to_matching_character(b, e); b == e)
                        return arguments;

                    break;

                case remark_start:
                    if (auto n = std::next(b); !try_arg_inserter(arg_start_it, b, state) && n != e
                        && symbols[static_cast<unsigned char>(*n)] == remark_start)
                    {
                        b = n;
                        next_state = state;
                    }

                    break;

                case other_char:
                    try_arg_inserter(arg_start_it, b, state);
                    break;

                default:
                    assert(false);
                    break;
            }

            ++b;
        }

        try_arg_inserter(arg_start_it, b, next_state);

        return arguments;
    }
};

struct consuming_regex_details
{
    bool needs_same_line;
    bool tolerate_no_space_at_end;
    const std::regex r;

    consuming_regex_details(const std::initializer_list<std::string_view>& words_to_consume,
        bool needs_same_line,
        bool tolerate_no_space_at_end)
        : needs_same_line(needs_same_line)
        , tolerate_no_space_at_end(tolerate_no_space_at_end)
        , r(get_consuming_regex(words_to_consume, tolerate_no_space_at_end))
    {}

private:
    static std::regex get_consuming_regex(
        const std::initializer_list<std::string_view>& words, bool tolerate_no_space_at_end)
    {
        assert(words.size());

        auto w_it = words.begin();

        std::string s = "(";
        s.append(*w_it++);
        while (w_it != words.end())
            s.append("(?:[ ]|--)+(?:").append(*w_it++).append(")");

        s.append(")([ ]|--)");
        if (tolerate_no_space_at_end)
            s.append("*");
        else
            s.append("+");
        s.append("(.*)");

        return std::regex(s);
    }
};

class db2_preprocessor final : public preprocessor // TODO Take DBCS into account
{
    std::string m_version;
    bool m_conditional;
    library_fetcher m_libs;
    diagnostic_op_consumer* m_diags = nullptr;
    std::vector<document_line> m_result;
    bool m_source_translated = false;
    semantics::source_info_processor& m_src_proc;
    db2_logical_line_helper m_ll_helper;
    db2_logical_line_helper m_ll_include_helper;

    enum class line_type
    {
        ignore,
        exec_sql,
        include,
        sql_type
    };

    void push_sql_version_data()
    {
        assert(!m_version.empty());

        constexpr auto version_chunk = (size_t)32;
        if (m_version.size() <= version_chunk)
        {
            m_result.emplace_back(replaced_line { "SQLVERSP DC    CL4'VER.' VERSION-ID PREFIX\n" });
            m_result.emplace_back(replaced_line { concat("SQLVERD1 DC    CL64'", m_version, "'        VERSION-ID\n") });
        }
        else
        {
            m_result.emplace_back(replaced_line { "SQLVERS  DS    CL68      VERSION-ID\n" });
            m_result.emplace_back(replaced_line { "         ORG   SQLVERS+0\n" });
            m_result.emplace_back(replaced_line { "SQLVERSP DC    CL4'VER.' VERS-ID PREFIX\n" });

            for (auto [version, i] = std::pair(std::string_view(m_version), 1); !version.empty();
                 version.remove_prefix(std::min(version.size(), version_chunk)), ++i)
            {
                auto i_str = std::to_string(i);
                m_result.emplace_back(replaced_line { concat("SQLVERD",
                    i_str,
                    " DC    CL32'",
                    version.substr(0, version_chunk),
                    "'    VERS-ID PART-",
                    i_str,
                    "\n") });
            }
        }
    }

    void push_sql_working_storage()
    {
        if (!m_version.empty())
            push_sql_version_data();

        m_result.emplace_back(replaced_line { "***$$$ SQL WORKING STORAGE                      \n" });
        m_result.emplace_back(replaced_line { "SQLDSIZ  DC    A(SQLDLEN) SQLDSECT SIZE         \n" });
        m_result.emplace_back(replaced_line { "SQLDSECT DSECT                                  \n" });
        m_result.emplace_back(replaced_line { "SQLTEMP  DS    CL128     TEMPLATE               \n" });
        m_result.emplace_back(replaced_line { "DSNTEMP  DS    F         INT SCROLL VALUE       \n" });
        m_result.emplace_back(replaced_line { "DSNTMP2  DS    PL16      DEC SCROLL VALUE       \n" });
        m_result.emplace_back(replaced_line { "DSNNROWS DS    F         MULTI-ROW N-ROWS VALUE \n" });
        m_result.emplace_back(replaced_line { "DSNNTYPE DS    H         MULTI-ROW N-ROWS TYPE  \n" });
        m_result.emplace_back(replaced_line { "DSNNLEN  DS    H         MULTI-ROW N-ROWS LENGTH\n" });
        m_result.emplace_back(replaced_line { "DSNPARMS DS    4F        DSNHMLTR PARM LIST     \n" });
        m_result.emplace_back(replaced_line { "DSNPNM   DS    CL386     PROCEDURE NAME         \n" });
        m_result.emplace_back(replaced_line { "DSNCNM   DS    CL128     CURSOR NAME            \n" });
        m_result.emplace_back(replaced_line { "SQL_FILE_READ      EQU 2                        \n" });
        m_result.emplace_back(replaced_line { "SQL_FILE_CREATE    EQU 8                        \n" });
        m_result.emplace_back(replaced_line { "SQL_FILE_OVERWRITE EQU 16                       \n" });
        m_result.emplace_back(replaced_line { "SQL_FILE_APPEND    EQU 32                       \n" });
        m_result.emplace_back(replaced_line { "         DS    0D                               \n" });
        m_result.emplace_back(replaced_line { "SQLPLIST DS    F                                \n" });
        m_result.emplace_back(replaced_line { "SQLPLLEN DS    H         PLIST LENGTH           \n" });
        m_result.emplace_back(replaced_line { "SQLFLAGS DS    XL2       FLAGS                  \n" });
        m_result.emplace_back(replaced_line { "SQLCTYPE DS    H         CALL-TYPE              \n" });
        m_result.emplace_back(replaced_line { "SQLPROGN DS    CL8       PROGRAM NAME           \n" });
        m_result.emplace_back(replaced_line { "SQLTIMES DS    CL8       TIMESTAMP              \n" });
        m_result.emplace_back(replaced_line { "SQLSECTN DS    H         SECTION                \n" });
        m_result.emplace_back(replaced_line { "SQLCODEP DS    A         CODE POINTER           \n" });
        m_result.emplace_back(replaced_line { "SQLVPARM DS    A         VPARAM POINTER         \n" });
        m_result.emplace_back(replaced_line { "SQLAPARM DS    A         AUX PARAM PTR          \n" });
        m_result.emplace_back(replaced_line { "SQLSTNM7 DS    H         PRE_V8 STATEMENT NUMBER\n" });
        m_result.emplace_back(replaced_line { "SQLSTYPE DS    H         STATEMENT TYPE         \n" });
        m_result.emplace_back(replaced_line { "SQLSTNUM DS    F         STATEMENT NUMBER       \n" });
        m_result.emplace_back(replaced_line { "SQLFLAG2 DS    H         internal flags         \n" });
        m_result.emplace_back(replaced_line { "SQLRSRVD DS    CL18      RESERVED               \n" });
        m_result.emplace_back(replaced_line { "SQLPVARS DS    CL8,F,2H,0CL44                   \n" });
        m_result.emplace_back(replaced_line { "SQLAVARS DS    CL8,F,2H,0CL44                   \n" });
        m_result.emplace_back(replaced_line { "         DS    0D                               \n" });
        m_result.emplace_back(replaced_line { "SQLDLEN  EQU   *-SQLDSECT                       \n" });
    }

    void inject_SQLCA()
    {
        m_result.emplace_back(replaced_line { "***$$$ SQLCA                          \n" });
        m_result.emplace_back(replaced_line { "SQLCA    DS    0F                     \n" });
        m_result.emplace_back(replaced_line { "SQLCAID  DS    CL8      ID            \n" });
        m_result.emplace_back(replaced_line { "SQLCABC  DS    F        BYTE COUNT    \n" });
        m_result.emplace_back(replaced_line { "SQLCODE  DS    F        RETURN CODE   \n" });
        m_result.emplace_back(replaced_line { "SQLERRM  DS    H,CL70   ERR MSG PARMS \n" });
        m_result.emplace_back(replaced_line { "SQLERRP  DS    CL8      IMPL-DEPENDENT\n" });
        m_result.emplace_back(replaced_line { "SQLERRD  DS    6F                     \n" });
        m_result.emplace_back(replaced_line { "SQLWARN  DS    0C       WARNING FLAGS \n" });
        m_result.emplace_back(replaced_line { "SQLWARN0 DS    C'W' IF ANY            \n" });
        m_result.emplace_back(replaced_line { "SQLWARN1 DS    C'W' = WARNING         \n" });
        m_result.emplace_back(replaced_line { "SQLWARN2 DS    C'W' = WARNING         \n" });
        m_result.emplace_back(replaced_line { "SQLWARN3 DS    C'W' = WARNING         \n" });
        m_result.emplace_back(replaced_line { "SQLWARN4 DS    C'W' = WARNING         \n" });
        m_result.emplace_back(replaced_line { "SQLWARN5 DS    C'W' = WARNING         \n" });
        m_result.emplace_back(replaced_line { "SQLWARN6 DS    C'W' = WARNING         \n" });
        m_result.emplace_back(replaced_line { "SQLWARN7 DS    C'W' = WARNING         \n" });
        m_result.emplace_back(replaced_line { "SQLEXT   DS    0CL8                   \n" });
        m_result.emplace_back(replaced_line { "SQLWARN8 DS    C                      \n" });
        m_result.emplace_back(replaced_line { "SQLWARN9 DS    C                      \n" });
        m_result.emplace_back(replaced_line { "SQLWARNA DS    C                      \n" });
        m_result.emplace_back(replaced_line { "SQLSTATE DS    CL5                    \n" });
        m_result.emplace_back(replaced_line { "***$$$\n" });
    }
    void inject_SQLDA()
    {
        m_result.emplace_back(replaced_line { "***$$$ SQLDA                                            \n" });
        m_result.emplace_back(replaced_line { "SQLTRIPL EQU    C'3'                                    \n" });
        m_result.emplace_back(replaced_line { "SQLDOUBL EQU    C'2'                                    \n" });
        m_result.emplace_back(replaced_line { "SQLSINGL EQU    C' '                                    \n" });
        m_result.emplace_back(replaced_line { "*                                                       \n" });
        m_result.emplace_back(replaced_line { "         SQLSECT SAVE                                   \n" });
        m_result.emplace_back(replaced_line { "*                                                       \n" });
        m_result.emplace_back(replaced_line { "SQLDA    DSECT                                          \n" });
        m_result.emplace_back(replaced_line { "SQLDAID  DS    CL8      ID                              \n" });
        m_result.emplace_back(replaced_line { "SQLDABC  DS    F        BYTE COUNT                      \n" });
        m_result.emplace_back(replaced_line { "SQLN     DS    H        COUNT SQLVAR/SQLVAR2 ENTRIES    \n" });
        m_result.emplace_back(replaced_line { "SQLD     DS    H        COUNT VARS (TWICE IF USING BOTH)\n" });
        m_result.emplace_back(replaced_line { "*                                                       \n" });
        m_result.emplace_back(replaced_line { "SQLVAR   DS    0F       BEGIN VARS                      \n" });
        m_result.emplace_back(replaced_line { "SQLVARN  DSECT ,        NTH VARIABLE                    \n" });
        m_result.emplace_back(replaced_line { "SQLTYPE  DS    H        DATA TYPE CODE                  \n" });
        m_result.emplace_back(replaced_line { "SQLLEN   DS    0H       LENGTH                          \n" });
        m_result.emplace_back(replaced_line { "SQLPRCSN DS    X        DEC PRECISION                   \n" });
        m_result.emplace_back(replaced_line { "SQLSCALE DS    X        DEC SCALE                       \n" });
        m_result.emplace_back(replaced_line { "SQLDATA  DS    A        ADDR OF VAR                     \n" });
        m_result.emplace_back(replaced_line { "SQLIND   DS    A        ADDR OF IND                     \n" });
        m_result.emplace_back(replaced_line { "SQLNAME  DS    H,CL30   DESCRIBE NAME                   \n" });
        m_result.emplace_back(replaced_line { "SQLVSIZ  EQU   *-SQLDATA                                \n" });
        m_result.emplace_back(replaced_line { "SQLSIZV  EQU   *-SQLVARN                                \n" });
        m_result.emplace_back(replaced_line { "*                                                       \n" });
        m_result.emplace_back(replaced_line { "SQLDA    DSECT                                          \n" });
        m_result.emplace_back(replaced_line { "SQLVAR2  DS     0F      BEGIN EXTENDED FIELDS OF VARS   \n" });
        m_result.emplace_back(replaced_line { "SQLVAR2N DSECT  ,       EXTENDED FIELDS OF NTH VARIABLE \n" });
        m_result.emplace_back(replaced_line { "SQLLONGL DS     F       LENGTH                          \n" });
        m_result.emplace_back(replaced_line { "SQLRSVDL DS     F       RESERVED                        \n" });
        m_result.emplace_back(replaced_line { "SQLDATAL DS     A       ADDR OF LENGTH IN BYTES         \n" });
        m_result.emplace_back(replaced_line { "SQLTNAME DS     H,CL30  DESCRIBE NAME                   \n" });
        m_result.emplace_back(replaced_line { "*                                                       \n" });
        m_result.emplace_back(replaced_line { "         SQLSECT RESTORE                                \n" });
        m_result.emplace_back(replaced_line { "***$$$\n" });
    }
    void inject_SQLSECT()
    {
        m_result.emplace_back(replaced_line { "         MACRO                          \n" });
        m_result.emplace_back(replaced_line { "         SQLSECT &TYPE                  \n" });
        m_result.emplace_back(replaced_line { "         GBLC  &SQLSECT                 \n" });
        m_result.emplace_back(replaced_line { "         AIF ('&TYPE' EQ 'RESTORE').REST\n" });
        m_result.emplace_back(replaced_line { "&SQLSECT SETC  '&SYSECT'                \n" });
        m_result.emplace_back(replaced_line { "         MEXIT                          \n" });
        m_result.emplace_back(replaced_line { ".REST    ANOP                           \n" });
        m_result.emplace_back(replaced_line { "&SQLSECT CSECT                          \n" });
        m_result.emplace_back(replaced_line { "         MEND                           \n" });
    }

    template<typename It>
    static std::optional<It> consume_words_advance_to_next(It& it, const It& it_e, const consuming_regex_details& crd)
    {
        if (std::match_results<It> matches; std::regex_match(it, it_e, matches, crd.r)
            && (!crd.needs_same_line || utils::text_matchers::same_line(matches[1].first, std::prev(matches[1].second)))
            && (!crd.tolerate_no_space_at_end || matches[2].length() || !matches[3].length()
                || (matches[1].second == matches[3].first
                    && !utils::text_matchers::same_line(std::prev(matches[1].second), matches[3].first))))
        {
            it = matches[3].first;
            return matches[1].second;
        }

        return std::nullopt;
    }

    template<typename It>
    std::optional<semantics::preproc_details::name_range> try_process_include(It it, const It& it_e, size_t lineno)
    {
        if (static const consuming_regex_details include_crd({ "INCLUDE" }, false, false);
            !consume_words_advance_to_next(it, it_e, include_crd))
            return std::nullopt;

        It inc_it_s;
        It inc_it_e;
        semantics::preproc_details::name_range nr;
        static const auto member_pattern = std::regex("(.*?)(?:[ ]|--)*$");

        for (auto reg_it = std::regex_iterator<It>(it, it_e, member_pattern), reg_it_e = std::regex_iterator<It>();
             reg_it != reg_it_e;
             ++reg_it)
        {
            if (const auto& sub_match = (*reg_it)[1]; sub_match.length())
            {
                if (nr.name.empty())
                    inc_it_s = sub_match.first;
                inc_it_e = sub_match.second;

                if (!nr.name.empty())
                    nr.name.push_back(' ');
                nr.name.append(sub_match.str());
            }
        }

        if (!nr.name.empty())
            nr.r = semantics::text_range(inc_it_s, inc_it_e, lineno);

        return nr;
    }

    [[nodiscard]] utils::value_task<std::pair<line_type, std::string>> process_include_member(
        line_type instruction_type, std::string member, size_t lineno)
    {
        auto member_upper = utils::to_upper_copy(member);

        if (member_upper == "SQLCA")
        {
            inject_SQLCA();
            co_return { instruction_type, member_upper };
        }
        if (member_upper == "SQLDA")
        {
            inject_SQLDA();
            co_return { instruction_type, member_upper };
        }
        m_result.emplace_back(replaced_line { "***$$$\n" });

        std::optional<std::pair<std::string, utils::resource::resource_location>> include_member;
        if (m_libs)
            include_member = co_await m_libs(member_upper);
        if (!include_member.has_value())
        {
            if (m_diags)
                m_diags->add_diagnostic(diagnostic_op::error_DB002(range(position(lineno, 0)), member));
            co_return { instruction_type, member };
        }

        auto& [include_mem_text, include_mem_loc] = *include_member;
        document d(include_mem_text);
        d.convert_to_replaced();
        co_await generate_replacement(d.begin(), d.end(), m_ll_include_helper, false);
        append_included_member(std::make_unique<included_member_details>(included_member_details {
            std::move(member_upper), std::move(include_mem_text), std::move(include_mem_loc) }));
        co_return { line_type::include, member };
    }

    static bool is_end(std::string_view s) { return utils::consume(s, "END") && (s.empty() || s.front() == ' '); }

    static std::string_view create_line_preview(std::string_view input)
    {
        const auto begin_offset = lexing::default_ictl.begin - 1;
        using namespace std::literals;
        if (input.size() < begin_offset)
            return {};

        input = input.substr(begin_offset, lexing::default_ictl.end - begin_offset);

        if (const auto rn = input.find_first_of("\r\n"sv); rn != std::string_view::npos)
            input = input.substr(0, rn);

        return input;
    }

    static bool ignore_line(std::string_view s) { return s.empty() || s.front() == '*' || s.substr(0, 2) == ".*"; }

    static semantics::preproc_details::name_range extract_label(std::string_view& s, size_t lineno)
    {
        auto label = utils::next_nonblank_sequence(s);
        if (!label.length())
            return {};

        s.remove_prefix(label.length());

        return semantics::preproc_details::name_range {
            std::string(label),
            range(position(lineno, 0), position(lineno, label.length())),
        };
    }

    static std::pair<line_type, semantics::preproc_details::name_range> extract_instruction(
        const std::string_view& line_preview, size_t lineno, size_t column)
    {
        static const std::pair<line_type, semantics::preproc_details::name_range> ignore(line_type::ignore, {});

        if (line_preview.empty())
            return ignore;

        const auto consume_and_create = [&line_preview, lineno, column](line_type line,
                                            const consuming_regex_details& crd,
                                            std::string_view line_id) {
            auto it = line_preview.begin();
            if (auto consumed_words_end = consume_words_advance_to_next(it, line_preview.end(), crd);
                consumed_words_end)
                return std::make_pair(line,
                    semantics::preproc_details::name_range { std::string(line_id),
                        range {
                            position(lineno, column),
                            position(lineno, column + std::ranges::distance(line_preview.begin(), *consumed_words_end)),
                        } });
            return ignore;
        };

        static const consuming_regex_details exec_sql_crd({ "EXEC", "SQL" }, true, false);
        static const consuming_regex_details sql_type_crd({ "SQL", "TYPE" }, true, false);

        switch (line_preview.front())
        {
            case 'E':
                return consume_and_create(line_type::exec_sql, exec_sql_crd, "EXEC SQL");

            case 'S':
                return consume_and_create(line_type::sql_type, sql_type_crd, "SQL TYPE");

            default:
                return ignore;
        }
    }

    void add_ds_line(std::string_view label, std::string_view label_suffix, std::string_view type, bool align = true)
    {
        m_result.emplace_back(replaced_line { concat(label,
            label_suffix,
            std::string(
                align && label.size() + label_suffix.size() < 8 ? 8 - (label.size() + label_suffix.size()) : 0, ' '),
            " DS ",
            std::string(align ? 2 + (type.front() != '0') : 0, ' '),
            type,
            "\n") });
    };

    struct lob_info_t
    {
        unsigned long long scale;
        unsigned long long limit;
        std::string prefix;
    };

    static lob_info_t lob_info(char type, char scale)
    {
        lob_info_t result;
        switch (scale)
        {
            case 'K':
                result.scale = 1024ULL;
                break;
            case 'M':
                result.scale = 1024ULL * 1024;
                break;
            case 'G':
                result.scale = 1024ULL * 1024 * 1024;
                break;
            default:
                result.scale = 1ULL;
                break;
        }
        switch (type)
        {
            case 'B':
                result.limit = 65535;
                result.prefix = "CL";
                break;
            case 'C':
                result.limit = 65535;
                result.prefix = "CL";
                break;
            case 'D':
                result.limit = 65534;
                result.prefix = "GL";
                break;
        }
        return result;
    }

    bool handle_lob(std::string_view label, char type_start, char type_end, char scale, long long size)
    {
        switch (type_end)
        {
            case 'E': // ..._FILE
                add_ds_line(label, "", "0FL4");
                add_ds_line(label, "_NAME_LENGTH", "FL4", false);
                add_ds_line(label, "_DATA_LENGTH", "FL4", false);
                add_ds_line(label, "_FILE_OPTIONS", "FL4", false);
                add_ds_line(label, "_NAME", "CL255", false);
                break;

            case 'R': // ..._LOCATOR
                add_ds_line(label, "", "FL4");
                break;

            default: {
                const auto li = lob_info(type_start, scale);
                auto len = size * li.scale;

                add_ds_line(label, "", "0FL4");
                add_ds_line(label, "_LENGTH", "FL4", false);
                add_ds_line(label, "_DATA", li.prefix + std::to_string(len <= li.limit ? len : li.limit), false);
                if (len > li.limit)
                    m_result.emplace_back(replaced_line { concat(" ORG   *+(",
                        // there seems be this strange artificial limit
                        std::min(len - li.limit, 1073676289ULL),
                        ")\n") });
                break;
            }
        }
        return true;
    };

    template<typename It>
    bool handle_r_starting_operands(const std::string_view& label, const It& it_b, const It& it_e)
    {
        auto ds_line_inserter = [&label, &it_e, this](
                                    It it, const consuming_regex_details& crd, std::string_view ds_line_type) {
            if (!consume_words_advance_to_next(it, it_e, crd))
                return false;
            add_ds_line(label, "", ds_line_type);
            return true;
        };

        assert(it_b != it_e && *it_b == 'R');

        static const consuming_regex_details result_set_crd({ "RESULT_SET_LOCATOR", "VARYING" }, false, true);
        static const consuming_regex_details rowid_crd({ "ROWID" }, false, true);

        if (auto it_n = std::next(it_b); it_n == it_e || (*it_n != 'E' && *it_n != 'O'))
            return false;
        else if (*it_n == 'E')
            return ds_line_inserter(it_b, result_set_crd, "FL4");
        else
            return ds_line_inserter(it_b, rowid_crd, "H,CL40");
    };

    template<typename It>
    bool process_sql_type_operands(const std::string_view& label, const It& it, const It& it_e)
    {
        if (it == it_e)
            return false;

        std::optional<std::pair<It, It>> type;
        std::optional<std::pair<It, It>> blob_size;
        std::optional<std::pair<It, It>> blob_unit;

        namespace m = utils::text_matchers;
        using string_matcher = m::basic_string_matcher<true, true>;
        static constexpr auto line_comment = m::seq(string_matcher("--"), m::start_of_next_line());
        static constexpr auto separator = m::plus(m::alt(m::space_matcher<false, false>(), line_comment));
        static constexpr auto xml_start = m::seq<string_matcher>("XML", separator, "AS", separator);
        static constexpr auto lob_file = m::alt<string_matcher>("BLOB_FILE", "CLOB_FILE", "DBCLOB_FILE");
        static constexpr auto lob_locator = m::alt<string_matcher>("BLOB_LOCATOR", "CLOB_LOCATOR", "DBCLOB_LOCATOR");
        static constexpr auto lob_rest_base = m::alt<string_matcher>("BLOB",
            "CLOB",
            "DBCLOB",
            m::seq<string_matcher>(
                m::alt<string_matcher>("BINARY", "CHARACTER", "CHAR"), separator, "LARGE", separator, "OBJECT"));

        const auto lob_size = m::seq(m::capture(blob_size, m::times<1, 9>(m::char_matcher("0123456789"))),
            m::opt(m::capture(blob_unit, m::char_matcher("KMG"))));

        const auto xml_type = m::seq<string_matcher>(xml_start,
            m::alt(m::capture(type, lob_file), m::seq(m::capture(type, lob_rest_base), separator, lob_size)));
        const auto lob_type = m::alt(m::capture(type, m::alt(lob_file, lob_locator)),
            m::seq(m::capture(type, lob_rest_base), separator, lob_size));
        const auto lob_or_xml_type = m::seq(m::alt(xml_type, lob_type), m::alt(m::end(), separator));

        switch (*it)
        {
            case 'R':
                return handle_r_starting_operands(label, it, it_e);

            case 'T': {
                static constexpr auto double_apo = m::basic_string_matcher<true, false>("''");
                static constexpr auto quoted_name = m::seq(
                    m::char_matcher("'"), m::star(m::alt(m::not_char_matcher("'"), double_apo)), m::char_matcher("'"));
                static constexpr auto name_without_quotes = m::seq(
                    m::not_char_matcher("' "), m::star(m::alt(line_comment, m::not_char_matcher("' "), double_apo)));

                static constexpr auto matcher = m::seq<string_matcher>("TABLE",
                    separator,
                    "LIKE",
                    separator,
                    m::alt(quoted_name, name_without_quotes),
                    separator,
                    "AS",
                    separator,
                    "LOCATOR",
                    m::alt(m::end(), separator));

                if (auto wrk = it; !matcher(wrk, it_e))
                    return false;

                add_ds_line(label, "", "FL4");
                return true;
            }

            case 'B':
            case 'C':
            case 'D':
            case 'X': {
                if (auto wrk = it; !lob_or_xml_type(wrk, it_e))
                    return false;
                char type_start = *type.value().first;
                char type_end = *std::prev(type.value().second);
                char scale = blob_unit.has_value() ? *blob_unit->first : 0;
                long long size = 0;
                if (blob_size.has_value())
                    size = std::stoll(std::string(blob_size->first, blob_size->second));
                return handle_lob(label, type_start, type_end, scale, size);
            }
            default:
                return false;
        }
    }

    void process_regular_line(const std::vector<lexing::logical_line_segment<std::string_view::iterator>>& ll_segments,
        std::string_view label)
    {
        if (!label.empty())
            m_result.emplace_back(replaced_line { concat(label, " DS 0H\n") });

        m_result.emplace_back(replaced_line { "***$$$\n" });

        bool first_line = true;
        for (const auto& segment : ll_segments)
        {
            std::string this_line(segment.begin, segment.end);

            if (std::exchange(first_line, false))
            {
                const auto appended_line_size = std::ranges::distance(segment.begin, segment.end);
                if (!label.empty())
                    this_line.replace(this_line.size() - appended_line_size,
                        label.size(),
                        label.size(),
                        ' '); // mask out any label-like characters
                this_line[this_line.size() - appended_line_size] = '*';
            }

            this_line.append("\n");
            m_result.emplace_back(replaced_line { std::move(this_line) });
        }
    }

    void process_sql_type_line(const db2_logical_line_helper& ll, std::string_view label, size_t instruction_end)
    {
        const auto diag_adder = [diags = m_diags](diagnostic_op&& diag) {
            if (diags)
                diags->add_diagnostic(std::move(diag));
        };

        m_result.emplace_back(replaced_line { "***$$$\n" });
        m_result.emplace_back(replaced_line { concat("*",
            std::string_view(ll.m_orig_ll.segments.front().code, ll.m_orig_ll.segments.front().continuation),
            "\n") });
        m_result.emplace_back(replaced_line { "***$$$\n" });

        // DB2 preprocessor exhibits strange behavior when SQL TYPE line is continued
        if (ll.m_db2_ll.segments.size() > 1)
            diag_adder(diagnostic_op::warn_DB005(range(position(ll.m_lineno, 0))));

        auto [it_b, it_e] = skip_to_operands(ll.m_db2_ll.begin(), ll.m_db2_ll.end(), instruction_end);
        if (static const consuming_regex_details is_crd({ "IS" }, true, true);
            !consume_words_advance_to_next(it_b, it_e, is_crd))
        {
            diag_adder(diagnostic_op::warn_DB006(range(position(ll.m_lineno, 0))));
            return;
        }

        if (label.empty())
            label = " "; // best matches the observed behavior
        if (!process_sql_type_operands(label, it_b, it_e))
            diag_adder(diagnostic_op::error_DB004(range(position(ll.m_lineno, 0))));
    }

    std::tuple<line_type, semantics::preproc_details::name_range, semantics::preproc_details::name_range> check_line(
        std::string_view input, size_t lineno)
    {
        static const std::
            tuple<line_type, semantics::preproc_details::name_range, semantics::preproc_details::name_range>
                ignore(line_type::ignore, {}, {});
        std::string_view line_preview = create_line_preview(input);

        if (ignore_line(line_preview))
            return ignore;

        semantics::preproc_details::name_range label = extract_label(line_preview, lineno);

        auto trimmed = utils::trim_left(line_preview);
        if (!trimmed)
            return ignore;

        if (is_end(line_preview))
        {
            push_sql_working_storage();

            return ignore;
        }

        if (auto [instruction_type, instruction_nr] =
                extract_instruction(line_preview, lineno, label.r.end.column + trimmed);
            instruction_type != line_type::ignore)
            return { std::move(instruction_type), label, std::move(instruction_nr) };

        return ignore;
    }

    template<typename It>
    bool sql_has_codegen(const It& it, const It& it_e) const
    {
        // handles only the most obvious cases (imprecisely)
        static const auto no_code_statements = std::regex("^(?:DECLARE|WHENEVER|BEGIN"
                                                          "(?:[ ]|--)+"
                                                          "DECLARE"
                                                          "(?:[ ]|--)+"
                                                          "SECTION|END"
                                                          "(?:[ ]|--)+"
                                                          "DECLARE"
                                                          "(?:[ ]|--)+"
                                                          "SECTION)(?= |$)",
            std::regex_constants::icase);
        return !std::regex_search(it, it_e, no_code_statements);
    }

    void generate_sql_code_mock(size_t in_params)
    {
        // this function generates semi-realistic sql statement replacement code, because people do strange things...
        // <arguments> input parameters
        m_result.emplace_back(replaced_line { "         BRAS  15,*+56                     \n" });
        m_result.emplace_back(replaced_line { "         DC    H'0',X'0000',H'0'           \n" });
        m_result.emplace_back(replaced_line { "         DC    XL8'0000000000000000'       \n" });
        m_result.emplace_back(replaced_line { "         DC    XL8'0000000000000000',H'0'  \n" });
        m_result.emplace_back(replaced_line { "         DC    H'0,0,0',X'0000',H'0',9H'0' \n" });
        m_result.emplace_back(replaced_line { "         MVC   SQLPLLEN(24),0(15)          \n" });
        m_result.emplace_back(replaced_line { "         MVC   SQLSTNM7(28),24(15)         \n" });
        m_result.emplace_back(replaced_line { "         LA    15,SQLCA                    \n" });
        m_result.emplace_back(replaced_line { "         ST    15,SQLCODEP                 \n" });

        if (in_params == 0)
        {
            m_result.emplace_back(replaced_line { "         MVC   SQLVPARM,=XL4'00000000'     \n" });
        }
        else
        {
            m_result.emplace_back(replaced_line { "         LA    14,SQLPVARS+16              \n" });
            for (size_t i = 0; i < in_params; ++i)
            {
                if (i > 0)
                    m_result.emplace_back(replaced_line { "         LA    14,44(,14)                  \n" });
                m_result.emplace_back(replaced_line { "         LA    15,0                        \n" });
                m_result.emplace_back(replaced_line { "         ST    15,4(,14)                   \n" });
                m_result.emplace_back(replaced_line { "         MVC   0(2,14),=X'0000'            \n" });
                m_result.emplace_back(replaced_line { "         MVC   2(2,14),=H'0'               \n" });
                m_result.emplace_back(replaced_line { "         SLR   15,15                       \n" });
                m_result.emplace_back(replaced_line { "         ST    15,8(,14)                   \n" });
                m_result.emplace_back(replaced_line { "         SLR   15,15                       \n" });
                m_result.emplace_back(replaced_line { "         ST    15,12(,14)                  \n" });
            }
            m_result.emplace_back(replaced_line { "         LA    14,SQLPVARS                   \n" });
            m_result.emplace_back(replaced_line { "         MVC   0(8,14),=XL8'0000000000000000'\n" });
            m_result.emplace_back(replaced_line { "         MVC   8(4,14),=F'0'                 \n" });
            m_result.emplace_back(replaced_line { "         MVC   12(2,14),=H'0'                \n" });
            m_result.emplace_back(replaced_line { "         MVC   14(2,14),=H'0'                \n" });
            m_result.emplace_back(replaced_line { "         ST    14,SQLVPARM                   \n" });
        }
        m_result.emplace_back(replaced_line { "         MVC   SQLAPARM,=XL4'00000000'     \n" });

        m_result.emplace_back(replaced_line { "         LA    1,SQLPLLEN                  \n" });
        m_result.emplace_back(replaced_line { "         ST    1,SQLPLIST                  \n" });
        m_result.emplace_back(replaced_line { "         OI    SQLPLIST,X'80'              \n" });
        m_result.emplace_back(replaced_line { "         LA    1,SQLPLIST                  \n" });
        m_result.emplace_back(replaced_line { "         L     15,=V(DSNHLI)               \n" });
        m_result.emplace_back(replaced_line { "         BALR  14,15                       \n" });
    }

    void skip_process(line_iterator& it, line_iterator end)
    {
        static constexpr std::string_view PROCESS_LITERAL = "*PROCESS";
        for (; it != end; ++it)
        {
            const auto text = it->text();
            if (text.size() < PROCESS_LITERAL.size())
                break;
            if (text.size() > PROCESS_LITERAL.size() && text[PROCESS_LITERAL.size()] != ' ')
                break;
            if (!std::equal(
                    PROCESS_LITERAL.begin(), PROCESS_LITERAL.end(), text.begin(), [](unsigned char l, unsigned char r) {
                        return l == ::toupper(r);
                    }))
                break;

            m_result.push_back(*it);
        }
    }

    template<std::forward_iterator It>
    static std::pair<It, It> skip_to_operands(It b, It e, size_t instruction_end)
    {
        std::advance(b, instruction_end);
        db2_logical_line_helper::trim_left(b, e);

        return { b, e };
    }

    [[nodiscard]] utils::task generate_replacement(
        line_iterator it, line_iterator end, db2_logical_line_helper& ll, bool include_allowed)
    {
        bool skip_continuation = false;

        const auto diag_adder = [diags = m_diags](diagnostic_op&& diag) {
            if (diags)
                diags->add_diagnostic(std::move(diag));
        };

        while (it != end)
        {
            const auto text = it->text();
            if (skip_continuation)
            {
                m_result.emplace_back(*it++);
                skip_continuation = is_continued(text);
                continue;
            }

            auto lineno = it->lineno(); // TODO: needs to be addressed for chained preprocessors

            auto [instruction_type, label_nr, instruction_nr] = check_line(text, lineno.value_or(0));
            if (instruction_type == line_type::ignore)
            {
                m_result.emplace_back(*it++);
                skip_continuation = is_continued(text);
                continue;
            }

            m_source_translated = true;

            it = ll.reinit(it, end, lineno.value_or(0));

            if (ll.m_db2_ll.continuation_error)
                diag_adder(diagnostic_op::error_DB001(range(position(ll.m_lineno, 0))));

            std::vector<semantics::preproc_details::name_range> args;
            switch (instruction_type)
            {
                case line_type::exec_sql: {
                    auto [it_b, it_e] =
                        skip_to_operands(ll.m_db2_ll.begin(), ll.m_db2_ll.end(), instruction_nr.r.end.column);
                    process_regular_line(ll.m_db2_ll.segments, label_nr.name);
                    if (auto inc_member_details = try_process_include(it_b, it_e, ll.m_lineno);
                        inc_member_details.has_value())
                    {
                        if (inc_member_details->name.empty())
                        {
                            diag_adder(diagnostic_op::warn_DB007(range(position(ll.m_lineno, 0))));
                            break;
                        }

                        if (include_allowed)
                            std::tie(instruction_type, inc_member_details->name) = co_await process_include_member(
                                instruction_type, inc_member_details->name, ll.m_lineno);
                        else
                            diag_adder(
                                diagnostic_op::error_DB003(range(position(ll.m_lineno, 0)), inc_member_details->name));

                        args.emplace_back(std::move(*inc_member_details));
                    }
                    else
                    {
                        static constexpr mini_parser<lexing::logical_line<std::string_view::iterator>::const_iterator>
                            parser;
                        args = parser.get_args(it_b, it_e, ll.m_lineno);
                        if (sql_has_codegen(it_b, it_e))
                            generate_sql_code_mock(args.size());
                        m_result.emplace_back(replaced_line { "***$$$\n" });
                    }

                    break;
                }

                case line_type::sql_type:
                    process_sql_type_line(ll, label_nr.name, instruction_nr.r.end.column);
                    break;

                default:
                    break;
            }

            if (lineno.has_value())
            {
                auto stmt = std::make_shared<semantics::preprocessor_statement_si>(
                    semantics::preproc_details {
                        semantics::text_range(ll.m_orig_ll.begin(), ll.m_orig_ll.end(), ll.m_lineno),
                        std::move(label_nr),
                        std::move(instruction_nr) },
                    instruction_type == line_type::include);

                do_highlighting(*stmt, ll.m_orig_ll, m_src_proc);

                stmt->m_details.instruction.preproc_specific_r = stmt->m_details.instruction.nr.r;
                stmt->m_details.operands = std::move(args);
                set_statement(std::move(stmt));
            }
        }
    }

    // Inherited via preprocessor
    [[nodiscard]] utils::value_task<document> generate_replacement(document doc) override
    {
        reset();
        m_source_translated = false;
        m_result.clear();
        m_result.reserve(doc.size());

        auto it = doc.begin();
        const auto end = doc.end();

        skip_process(it, end);
        // ignores ICTL
        inject_SQLSECT();

        co_await generate_replacement(it, end, m_ll_helper, true);

        if (m_source_translated || !m_conditional)
            co_return document(std::move(m_result));
        else
            co_return doc;
    }

    void do_highlighting(const semantics::preprocessor_statement_si& stmt,
        const lexing::logical_line<std::string_view::iterator>& ll,
        semantics::source_info_processor& src_proc,
        size_t continue_column = 15) const override
    {
        preprocessor::do_highlighting(stmt, ll, src_proc, continue_column);

        for (size_t i = 0, lineno = stmt.m_details.stmt_r.start.line, line_start_column = 0;
             i < m_ll_helper.m_db2_ll.segments.size();
             ++i, ++lineno, std::exchange(line_start_column, continue_column))
        {
            const auto& segment = m_ll_helper.m_db2_ll.segments[i];
            auto comment_start_column = line_start_column + std::ranges::distance(segment.code, segment.continuation);

            if (const auto& comment = m_ll_helper.m_comments[i]; comment.has_value())
            {
                comment_start_column -= 2; // Compensate for code part having the '--' separator while comment part not
                src_proc.add_hl_symbol(token_info(range(position(lineno, comment_start_column),
                                                      position(lineno, comment_start_column + comment->length() + 2)),
                    hl_scopes::remark));
            }

            if (segment.code != segment.continuation)
                if (auto operand_start_column = i == 0 ? stmt.m_details.instruction.nr.r.end.column : continue_column;
                    operand_start_column < comment_start_column)
                    src_proc.add_hl_symbol(token_info(
                        range(position(lineno, operand_start_column), position(lineno, comment_start_column)),
                        hl_scopes::operand));
        }
    }

public:
    db2_preprocessor(const db2_preprocessor_options& opts,
        library_fetcher libs,
        diagnostic_op_consumer* diags,
        semantics::source_info_processor& src_proc)
        : m_version(opts.version)
        , m_conditional(opts.conditional)
        , m_libs(std::move(libs))
        , m_diags(diags)
        , m_src_proc(src_proc)
    {}
};
} // namespace

std::unique_ptr<preprocessor> preprocessor::create(const db2_preprocessor_options& opts,
    library_fetcher libs,
    diagnostic_op_consumer* diags,
    semantics::source_info_processor& src_proc)
{
    return std::make_unique<db2_preprocessor>(opts, std::move(libs), diags, src_proc);
}

} // namespace hlasm_plugin::parser_library::processing
