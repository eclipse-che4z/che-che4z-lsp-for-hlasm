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

#include <cassert>
#include <charconv>
#include <regex>
#include <utility>

#include "lexing/logical_line.h"
#include "preprocessor_options.h"
#include "processing/preprocessor.h"
#include "workspaces/parse_lib_provider.h"

namespace hlasm_plugin::parser_library::processing {

namespace {

const std::unordered_map<std::string_view, int> DFHRESP_operands = {
    { "NORMAL", 0 },
    { "ERROR", 1 },
    { "RDATT", 2 },
    { "WRBRK", 3 },
    { "EOF", 4 },
    { "EODS", 5 },
    { "EOC", 6 },
    { "INBFMH", 7 },
    { "ENDINPT", 8 },
    { "NONVAL", 9 },
    { "NOSTART", 10 },
    { "TERMIDERR", 11 },
    { "FILENOTFOUND", 12 },
    { "NOTFND", 13 },
    { "DUPREC", 14 },
    { "DUPKEY", 15 },
    { "INVREQ", 16 },
    { "IOERR", 17 },
    { "NOSPACE", 18 },
    { "NOTOPEN", 19 },
    { "ENDFILE", 20 },
    { "ILLOGIC", 21 },
    { "LENGERR", 22 },
    { "QZERO", 23 },
    { "SIGNAL", 24 },
    { "QBUSY", 25 },
    { "ITEMERR", 26 },
    { "PGMIDERR", 27 },
    { "TRANSIDERR", 28 },
    { "ENDDATA", 29 },
    { "INVTSREQ", 30 },
    { "EXPIRED", 31 },
    { "RETPAGE", 32 },
    { "RTEFAIL", 33 },
    { "RTESOME", 34 },
    { "TSIOERR", 35 },
    { "MAPFAIL", 36 },
    { "INVERRTERM", 37 },
    { "INVMPSZ", 38 },
    { "IGREQID", 39 },
    { "OVERFLOW", 40 },
    { "INVLDC", 41 },
    { "NOSTG", 42 },
    { "JIDERR", 43 },
    { "QIDERR", 44 },
    { "NOJBUFSP", 45 },
    { "DSSTAT", 46 },
    { "SELNERR", 47 },
    { "FUNCERR", 48 },
    { "UNEXPIN", 49 },
    { "NOPASSBKRD", 50 },
    { "NOPASSBKWR", 51 },
    { "SEGIDERR", 52 },
    { "SYSIDERR", 53 },
    { "ISCINVREQ", 54 },
    { "ENQBUSY", 55 },
    { "ENVDEFERR", 56 },
    { "IGREQCD", 57 },
    { "SESSIONERR", 58 },
    { "SYSBUSY", 59 },
    { "SESSBUSY", 60 },
    { "NOTALLOC", 61 },
    { "CBIDERR", 62 },
    { "INVEXITREQ", 63 },
    { "INVPARTNSET", 64 },
    { "INVPARTN", 65 },
    { "PARTNFAIL", 66 },
    { "USERIDERR", 69 },
    { "NOTAUTH", 70 },
    { "VOLIDERR", 71 },
    { "SUPPRESSED", 72 },
    { "RESIDERR", 75 },
    { "NOSPOOL", 80 },
    { "TERMERR", 81 },
    { "ROLLEDBACK", 82 },
    { "END", 83 },
    { "DISABLED", 84 },
    { "ALLOCERR", 85 },
    { "STRELERR", 86 },
    { "OPENERR", 87 },
    { "SPOLBUSY", 88 },
    { "SPOLERR", 89 },
    { "NODEIDERR", 90 },
    { "TASKIDERR", 91 },
    { "TCIDERR", 92 },
    { "DSNNOTFOUND", 93 },
    { "LOADING", 94 },
    { "MODELIDERR", 95 },
    { "OUTDESCRERR", 96 },
    { "PARTNERIDERR", 97 },
    { "PROFILEIDERR", 98 },
    { "NETNAMEIDERR", 99 },
    { "LOCKED", 100 },
    { "RECORDBUSY", 101 },
    { "UOWNOTFOUND", 102 },
    { "UOWLNOTFOUND", 103 },
    { "LINKABEND", 104 },
    { "CHANGED", 105 },
    { "PROCESSBUSY", 106 },
    { "ACTIVITYBUSY", 107 },
    { "PROCESSERR", 108 },
    { "ACTIVITYERR", 109 },
    { "CONTAINERERR", 110 },
    { "EVENTERR", 111 },
    { "TOKENERR", 112 },
    { "NOTFINISHED", 113 },
    { "POOLERR", 114 },
    { "TIMERERR", 115 },
    { "SYMBOLERR", 116 },
    { "TEMPLATERR", 117 },
    { "NOTSUPERUSER", 118 },
    { "CSDERR", 119 },
    { "DUPRES", 120 },
    { "RESUNAVAIL", 121 },
    { "CHANNELERR", 122 },
    { "CCSIDERR", 123 },
    { "TIMEDOUT", 124 },
    { "CODEPAGEERR", 125 },
    { "INCOMPLETE", 126 },
    { "APPNOTFOUND", 127 },
    { "BUSY", 128 },
};

// emulates limited variant of alternative operand parser and performs DFHRESP substitutions
// recognizes L' attribute, '...' strings and skips end of line comments
template<typename It>
class mini_parser
{
    It m_begin;
    It m_end;
    std::string substituted_operands;
    int valid_dfhresp = 0;

    int next() const
    {
        auto n = std::next(m_begin);
        if (n == m_end)
            return -1;
        return *n;
    }

    enum class symbol_type : unsigned char
    {
        normal,
        blank,
        apostrophe,
        comma,
        operator_symbol,
    };

    static constexpr std::array symbols = []() {
        std::array<symbol_type, std::numeric_limits<unsigned char>::max() + 1> r {};

        r[(unsigned char)' '] = symbol_type::blank;
        r[(unsigned char)'\''] = symbol_type::apostrophe;
        r[(unsigned char)','] = symbol_type::comma;

        r[(unsigned char)'*'] = symbol_type::operator_symbol;
        r[(unsigned char)'.'] = symbol_type::operator_symbol;
        r[(unsigned char)'-'] = symbol_type::operator_symbol;
        r[(unsigned char)'+'] = symbol_type::operator_symbol;
        r[(unsigned char)'='] = symbol_type::operator_symbol;
        r[(unsigned char)'<'] = symbol_type::operator_symbol;
        r[(unsigned char)'>'] = symbol_type::operator_symbol;
        r[(unsigned char)'('] = symbol_type::operator_symbol;
        r[(unsigned char)')'] = symbol_type::operator_symbol;
        r[(unsigned char)'/'] = symbol_type::operator_symbol;
        r[(unsigned char)'&'] = symbol_type::operator_symbol;
        r[(unsigned char)'|'] = symbol_type::operator_symbol;

        return r;
    }();

    template<typename T>
    std::true_type same_line_detector(const T& t, decltype(t.same_line(t)) = false);
    std::false_type same_line_detector(...);

    bool same_line(It l, It r)
    {
        if constexpr (decltype(same_line_detector(l))::value)
            return l.same_line(r);
        else
            return true;
    }

public:
    mini_parser(It b, It e)
        : m_begin(b)
        , m_end(e)
    {}

    std::string operands() && { return std::move(substituted_operands); }

    int parse_and_substitue()
    {
        std::match_results<It> matches;
        bool next_last_attribute = false;
        bool next_new_token = true;
        while (m_begin != m_end)
        {
            const bool last_attribute = std::exchange(next_last_attribute, false);
            const bool new_token = std::exchange(next_new_token, false);
            const char c = *m_begin;
            const symbol_type s = symbols[(unsigned char)c];

            switch (s)
            {
                case symbol_type::normal:
                    if (!new_token)
                        break;
                    if (c == 'L' || c == 'l')
                    {
                        if (next() == '\'')
                        {
                            substituted_operands.push_back(c);
                            substituted_operands.push_back('\'');
                            ++m_begin;
                            ++m_begin;
                            next_last_attribute = true;
                            next_new_token = true;
                            continue;
                        }
                    }
                    else if (!last_attribute && (c == 'D' || c == 'd'))
                    {
                        // check for DFHRESP expression
                        static const std::regex DFHRESP_matcher(
                            []() {
                                std::string list;
                                for (const auto& [key, value] : DFHRESP_operands)
                                    list.append(key).append(1, '|');
                                // keep the empty alternative
                                return "^DFHRESP[ ]*\\([ ]*(" + list + ")[ ]*\\)";
                            }(),
                            std::regex_constants::icase);
                        if (std::regex_search(m_begin, m_end, matches, DFHRESP_matcher))
                        {
                            if (matches[1].length() == 0)
                                return -1; // indicate NULL argument error

                            substituted_operands.append("=F'")
                                .append(std::to_string(DFHRESP_operands.at(context::to_upper_copy(matches[1].str()))))
                                .append("'");
                            m_begin = matches.suffix().first;
                            ++valid_dfhresp;
                            continue;
                        }
                    }
                    break;

                case symbol_type::blank:
                    // everything that follows is a comment
                    goto done;

                case symbol_type::apostrophe:
                    // read string literal
                    next_new_token = true;
                    do
                    {
                        substituted_operands.push_back(*m_begin);
                        ++m_begin;
                        if (m_begin == m_end)
                            goto done;
                    } while (*m_begin != '\'');
                    break;

                case symbol_type::comma:
                    next_new_token = true;
                    if (next() == ' ')
                    {
                        // skips comment at the end of the line
                        substituted_operands.push_back(c);
                        auto skip_line = m_begin;
                        while (skip_line != m_end && same_line(m_begin, skip_line))
                            ++skip_line;
                        m_begin = skip_line;
                        continue;
                    }
                    break;

                case symbol_type::operator_symbol:
                    next_new_token = true;
                    break;

                default:
                    assert(false);
                    break;
            }
            substituted_operands.push_back(c);
            ++m_begin;
        }

    done:
        return valid_dfhresp;
    }
};

class cics_preprocessor : public preprocessor
{
    const char* m_last_position = nullptr;
    lexing::logical_line m_logical_line;
    std::string m_operands;
    library_fetcher m_libs;
    diagnostic_op_consumer* m_diags = nullptr;
    std::string m_buffer;
    cics_preprocessor_options m_options;

    bool m_end_seen = false;
    bool m_global_macro_called = false;
    bool m_pending_prolog = false;
    bool m_pending_dfhresp_null_error = false;

    std::match_results<std::string_view::iterator> matches_sv;
    std::match_results<lexing::logical_line::const_iterator> matches_ll;

public:
    cics_preprocessor(const cics_preprocessor_options& options, library_fetcher libs, diagnostic_op_consumer* diags)
        : m_libs(std::move(libs))
        , m_diags(diags)
        , m_options(options)
    {}

    void inject_no_end_warning()
    {
        m_buffer.append("*DFH7041I W  NO END CARD FOUND - COPYBOOK ASSUMED.\n"
                        "         DFHEIMSG 4\n");
    }

    void inject_DFHEIGBL(bool rsect)
    {
        if (rsect)
        {
            if (m_options.leasm)
                m_buffer.append("         DFHEIGBL ,,RS,LE          INSERTED BY TRANSLATOR\n");
            else
                m_buffer.append("         DFHEIGBL ,,RS,NOLE        INSERTED BY TRANSLATOR\n");
        }
        else
        {
            if (m_options.leasm)
                m_buffer.append("         DFHEIGBL ,,,LE            INSERTED BY TRANSLATOR\n");
            else
                m_buffer.append("         DFHEIGBL ,,,NOLE          INSERTED BY TRANSLATOR\n");
        }
    }

    void inject_prolog() { m_buffer.append("         DFHEIENT                  INSERTED BY TRANSLATOR\n"); }
    void inject_dfhresp_null_error()
    {
        m_buffer.append("*DFH7218I S  SUB-OPERAND(S) OF 'DFHRESP' CANNOT BE NULL. COMMAND NOT\n");
        m_buffer.append("*            TRANSLATED.\n");
        m_buffer.append("         DFHEIMSG 12\n");
    }
    void inject_end_code()
    {
        if (m_options.epilog)
            m_buffer.append("         DFHEIRET                  INSERTED BY TRANSLATOR\n");
        if (m_options.prolog)
        {
            m_buffer.append("         DFHEISTG                  INSERTED BY TRANSLATOR\n");
            m_buffer.append("         DFHEIEND                  INSERTED BY TRANSLATOR\n");
        }
    }

    bool try_asm_xopts(std::string_view input, size_t lineno)
    {
        if (input.substr(0, 5) != "*ASM ")
            return false;

        auto [line, _] = lexing::extract_line(input);
        if (m_diags && line.size() > lexing::default_ictl.end && line[lexing::default_ictl.end] != ' ')
            m_diags->add_diagnostic(diagnostic_op::warn_CIC001(range(position(lineno, 0))));

        line = line.substr(0, lexing::default_ictl.end);

        static const std::regex asm_statement(R"(\*ASM[ ]+[Xx][Oo][Pp][Tt][Ss][(']([A-Z, ]*)[)'][ ]*)");
        static const std::regex op_sep("[ ,]+");
        static const std::unordered_map<std::string_view, std::pair<bool cics_preprocessor_options::*, bool>> opts {
            { "PROLOG", { &cics_preprocessor_options::prolog, true } },
            { "NOPROLOG", { &cics_preprocessor_options::prolog, false } },
            { "EPILOG", { &cics_preprocessor_options::epilog, true } },
            { "NOEPILOG", { &cics_preprocessor_options::epilog, false } },
            { "LEASM", { &cics_preprocessor_options::leasm, true } },
            { "NOLEASM", { &cics_preprocessor_options::leasm, false } },
        };

        std::match_results<std::string_view::const_iterator> m_regex_match;
        if (!std::regex_match(line.begin(), line.end(), m_regex_match, asm_statement) || m_regex_match[1].length() == 0)
            return false;

        std::string_view operands(&*m_regex_match[1].first, (size_t)m_regex_match[1].length());
        auto opts_begin =
            std::regex_token_iterator<std::string_view::iterator>(operands.begin(), operands.end(), op_sep, -1);
        auto opts_end = std::regex_token_iterator<std::string_view::iterator>();

        for (; opts_begin != opts_end; ++opts_begin)
        {
            if (opts_begin->length() == 0)
                continue;

            std::string_view name(&*opts_begin->first, opts_begin->second - opts_begin->first);
            if (auto o = opts.find(name); o != opts.end())
                (m_options.*o->second.first) = o->second.second;
        }

        return true;
    }

    void process_asm_statement(char type)
    {
        switch (type)
        {
            case 'S':
            case 'C':
                if (!std::exchange(m_global_macro_called, true))
                    inject_DFHEIGBL(false);
                m_pending_prolog = m_options.prolog;
                break;

            case 'R':
                m_global_macro_called = true;
                inject_DFHEIGBL(true);
                m_pending_prolog = m_options.prolog;
                break;

            case 'E':
                m_end_seen = true;
                inject_end_code();
                break;

            default:
                assert(false);
                break;
        }
    }

    static constexpr const lexing::logical_line_extractor_args cics_extract { 1, 71, 2, false, false };

    static constexpr size_t valid_cols = 1 + lexing::default_ictl.end - (lexing::default_ictl.begin - 1);
    static auto create_line_preview(std::string_view input)
    {
        return lexing::utf8_substr(lexing::extract_line(input).first, lexing::default_ictl.begin - 1, valid_cols);
    }

    static bool ignore_line(std::string_view s) { return s.empty() || s.front() == '*' || s.substr(0, 2) == ".*"; }

    struct label_info
    {
        size_t byte_length;
        size_t char_length;
    };

    template<typename It>
    void echo_text_and_inject_label(It b, It e, const label_info& li)
    {
        // print lines, remove continuation character and label on the first line
        bool first_line = true;
        for (const auto& l : m_logical_line.segments)
        {
            auto buf_len = m_buffer.size();
            m_buffer.append(lexing::utf8_substr(l.line, 0, cics_extract.end).str);

            if (auto after_cont = lexing::utf8_substr(l.line, cics_extract.end + 1).str; !after_cont.empty())
                m_buffer.append(" ").append(after_cont);

            if (first_line)
                m_buffer.replace(buf_len, li.byte_length, li.char_length, ' ');

            m_buffer[buf_len] = '*';
            m_buffer.append("\n");
            first_line = false;
        }

        m_buffer.append(b, e);
        if (li.char_length <= 8)
            m_buffer.append(9 - li.char_length, ' ');
        else
            m_buffer.append(" DS 0H\n").append(9, ' ');
    }

    void process_exec_cics(const std::match_results<lexing::logical_line::const_iterator>& matches)
    {
        auto label_b = matches[1].first;
        auto label_e = matches[1].second;
        label_info li {
            (size_t)std::distance(label_b, label_e),
            (size_t)std::count_if(label_b, label_e, [](unsigned char c) { return (c & 0xc0) != 0x80; }),
        };
        echo_text_and_inject_label(label_b, label_e, li);

        m_buffer.append("DFHECALL =X'0E'\n"); // TODO: generate correct calls
    }

    int try_substituting_dfhresp(const std::match_results<lexing::logical_line::const_iterator>& matches)
    {
        mini_parser p(matches[3].first, matches[3].second);
        auto events = p.parse_and_substitue();
        if (events > 0)
        {
            auto label_b = matches[1].first;
            auto label_e = matches[1].second;
            label_info li {
                (size_t)std::distance(label_b, label_e),
                (size_t)std::count_if(label_b, label_e, [](unsigned char c) { return (c & 0xc0) != 0x80; }),
            };

            echo_text_and_inject_label(label_b, label_e, li);

            auto text_to_add = matches[2].str();
            if (auto instr_len = lexing::utf8_substr(text_to_add).char_count; instr_len < 4)
                text_to_add.append(4 - instr_len, ' ');
            text_to_add.append(1, ' ').append(std::move(p).operands());

            std::string_view t = text_to_add;

            size_t line_limit = 62;
            while (true)
            {
                auto part = lexing::utf8_substr(t, 0, line_limit);
                m_buffer.append(part.str);
                t.remove_prefix(part.str.size());
                if (t.empty())
                    break;

                m_buffer.append("*\n               ");

                line_limit = 56;
            }
            m_buffer.append(1, '\n');
        }

        return events;
    }

    /* returns number of consumed lines */
    size_t fill_buffer(std::string_view& input, size_t lineno)
    {
        if (std::exchange(m_pending_prolog, false))
            inject_prolog();
        if (std::exchange(m_pending_dfhresp_null_error, false))
            inject_dfhresp_null_error();

        if (input.empty())
        {
            if (!std::exchange(m_end_seen, true))
                inject_no_end_warning();
            return 0;
        }

        if (lineno == 0 && try_asm_xopts(input, lineno))
            return 0;

        auto [line, line_len_chars, _] = create_line_preview(input);

        if (ignore_line(line))
            return 0;

        // apparently lines full of characters are ignored
        if (line_len_chars == valid_cols && line.find(' ') == std::string_view::npos)
            return 0;

        static const std::regex line_of_interest("(?:[^ ]*)[ ]+(START|CSECT|RSECT|END)(?: .+)?");

        if (std::regex_match(line.begin(), line.end(), matches_sv, line_of_interest))
        {
            process_asm_statement(*matches_sv[1].first);
            return 0;
        }

        const std::string_view input_backup = input;

        bool extracted = lexing::extract_logical_line(m_logical_line, input, cics_extract);
        assert(extracted);
        bool exec_cics_continuation_error = false;
        if (m_logical_line.continuation_error)
        {
            exec_cics_continuation_error = true;
            // keep 1st line only
            m_logical_line.segments.erase(m_logical_line.segments.begin() + 1, m_logical_line.segments.end());
        }

        static const std::regex exec_cics("([^ ]*)[ ]+(?:[eE][xX][eE][cC][ ]+[cC][iI][cC][sS])(?: .+)?");

        if (std::regex_match(m_logical_line.begin(), m_logical_line.end(), matches_ll, exec_cics))
        {
            process_exec_cics(matches_ll);

            if (exec_cics_continuation_error)
            {
                if (m_diags)
                    m_diags->add_diagnostic(diagnostic_op::warn_CIC001(range(position(lineno, 0))));
                m_buffer.append("*DFH7080I W  CONTINUATION OF EXEC COMMAND IGNORED.\n"
                                "         DFHEIMSG 4\n");
            }

            return m_logical_line.segments.size();
        }

        static const std::regex dfhresp_lookup(
            "([^ ]*)[ ]+([A-Z#$@][A-Z#$@0-9]*)[ ]+(.*DFHRESP[ ]*\\([ ]*[A-Z]*[ ]*\\).*)", std::regex_constants::icase);

        input = input_backup;

        extracted = lexing::extract_logical_line(m_logical_line, input, lexing::default_ictl);
        assert(extracted);

        if (m_logical_line.continuation_error)
        {
            if (m_diags)
                m_diags->add_diagnostic(diagnostic_op::warn_CIC001(range(position(lineno, 0))));
        }
        else if (std::regex_match(m_logical_line.begin(), m_logical_line.end(), matches_ll, dfhresp_lookup))
        {
            switch (try_substituting_dfhresp(matches_ll))
            {
                case -1:
                    if (m_diags)
                        m_diags->add_diagnostic(diagnostic_op::warn_CIC002(range(position(lineno, 0))));
                    m_pending_dfhresp_null_error = true;
                    break;

                case 0:
                    break;

                default:
                    return m_logical_line.segments.size();
            }
        }

        input = input_backup;

        return 0;
    }

    // Inherited via preprocessor
    std::optional<std::string> generate_replacement(std::string_view& input, size_t& lineno) override
    {
        if (input.data() == m_last_position)
            return std::nullopt;

        m_buffer.clear();
        if (std::exchange(m_last_position, input.data()) == nullptr)
        {
            // nothing so far
        }

        lineno += fill_buffer(input, lineno);
        if (m_buffer.size())
            return m_buffer;
        else
            return std::nullopt;
    }

    bool finished() const override { return !m_pending_prolog && !m_pending_dfhresp_null_error; }

    cics_preprocessor_options current_options() const { return m_options; }
};

} // namespace

std::unique_ptr<preprocessor> preprocessor::create(
    const cics_preprocessor_options& options, library_fetcher libs, diagnostic_op_consumer* diags)
{
    return std::make_unique<cics_preprocessor>(options, std::move(libs), diags);
}

namespace test {
cics_preprocessor_options test_cics_current_options(const preprocessor& p)
{
    return static_cast<const cics_preprocessor&>(p).current_options();
}

std::pair<int, std::string> test_cics_miniparser(const std::vector<std::string_view>& list)
{
    lexing::logical_line ll;
    std::transform(list.begin(), list.end(), std::back_inserter(ll.segments), [](std::string_view s) {
        lexing::logical_line_segment lls;
        lls.code = s;
        return lls;
    });

    mini_parser p(ll.begin(), ll.end());
    std::pair<int, std::string> result;

    if ((result.first = p.parse_and_substitue()) >= 0)
        result.second = std::move(p).operands();

    return result;
}
} // namespace test

} // namespace hlasm_plugin::parser_library::processing
