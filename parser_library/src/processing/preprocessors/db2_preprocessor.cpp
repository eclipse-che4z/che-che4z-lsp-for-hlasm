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
#include <cassert>
#include <cctype>
#include <charconv>
#include <regex>
#include <span>
#include <tuple>
#include <utility>

#include "lexing/logical_line.h"
#include "preprocessor_options.h"
#include "processing/preprocessor.h"
#include "utils/concat.h"
#include "workspaces/parse_lib_provider.h"

namespace {
constexpr std::string_view trim_right(std::string_view s)
{
    const auto i = s.find_last_not_of(' ');

    if (i == std::string_view::npos)
        return s;

    return s.substr(0, i + 1);
}
} // namespace

namespace hlasm_plugin::parser_library::processing {
namespace {
using utils::concat;

class db2_preprocessor : public preprocessor
{
    lexing::logical_line m_logical_line;
    std::string m_operands;
    std::string m_version;
    bool m_conditional;
    library_fetcher m_libs;
    diagnostic_op_consumer* m_diags = nullptr;
    std::vector<document_line> m_result;
    bool m_source_translated = false;

    static bool remove_space(std::string_view& s)
    {
        if (s.empty() || s.front() != ' ')
            return false;
        const auto non_space = s.find_first_not_of(' ');

        if (non_space == std::string_view::npos)
        {
            s = {};
            return true;
        }

        s.remove_prefix(non_space);
        return true;
    }

    static bool consume(std::string_view& s, std::string_view lit)
    {
        // case sensitive
        if (s.substr(0, lit.size()) != lit)
            return false;
        s.remove_prefix(lit.size());
        return true;
    }

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

    void process_include(std::string_view operands, size_t lineno)
    {
        if (operands == "SQLCA")
        {
            inject_SQLCA();
            return;
        }
        if (operands == "SQLDA")
        {
            inject_SQLDA();
            return;
        }
        m_result.emplace_back(replaced_line { "***$$$\n" });

        std::optional<std::string> include_text;
        if (m_libs)
            include_text = m_libs(operands);
        if (!include_text.has_value())
        {
            if (m_diags)
                m_diags->add_diagnostic(diagnostic_op::error_DB002(range(position(lineno, 0)), operands));
            return;
        }

        document d(include_text.value());
        d.convert_to_replaced();
        generate_replacement(d.begin(), d.end(), false);
    }
    static bool consume_words(
        std::string_view& l, std::initializer_list<std::string_view> words, bool tolerate_no_space_at_end = false)
    {
        const auto init_l = l;
        for (const auto& w : words)
        {
            if (!consume(l, w))
            {
                l = init_l; // all or nothing
                return false;
            }
            if (!remove_space(l))
            {
                if (tolerate_no_space_at_end && l.empty() && &w == words.end() - 1)
                    return true;
                l = init_l; // all or nothing
                return false;
            }
        }
        return true;
    }

    static bool is_end(std::string_view s)
    {
        if (!consume(s, "END"))
            return false;

        if (s.empty() || s.front() == ' ')
            return true;

        return false;
    }

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

    static std::string_view extract_label(std::string_view& s)
    {
        if (s.empty() || s.front() == ' ')
            return {};

        auto space = s.find(' ');
        if (space == std::string_view::npos)
            space = s.size();

        std::string_view result = s.substr(0, space);

        s.remove_prefix(space);

        return result;
    }

    enum class line_type
    {
        ignore,
        exec_sql,
        include,
        sql_type
    };

    static line_type consume_instruction(std::string_view& line_preview)
    {
        if (line_preview.empty())
            return line_type::ignore;

        switch (line_preview.front())
        {
            case 'E':
                if (consume_words(line_preview, { "EXEC", "SQL" }))
                {
                    if (consume_words(line_preview, { "INCLUDE" }))
                        return line_type::include;
                    else
                        return line_type::exec_sql;
                }
                return line_type::ignore;

            case 'S':
                if (consume_words(line_preview, { "SQL", "TYPE", "IS" }))
                    return line_type::sql_type;
                return line_type::ignore;

            default:
                return line_type::ignore;
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

    bool handle_lob(const std::regex& pattern, std::string_view label, std::string_view operands)
    {
        std::match_results<std::string_view::const_iterator> match;
        if (!std::regex_match(operands.cbegin(), operands.cend(), match, pattern))
            return false;

        switch ((match[4].matched ? match[4] : match[1]).second[-1])
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
                const auto li = lob_info(*match[1].first, match[3].matched ? *match[3].first : 0);
                unsigned long long len;
                std::from_chars(&*match[2].first, &*match[2].first + match[2].length(), len);
                len *= li.scale;

                add_ds_line(label, "", "0FL4");
                add_ds_line(label, "_LENGTH", "FL4", false);
                add_ds_line(label, "_DATA", li.prefix + std::to_string(len <= li.limit ? len : li.limit), false);
                if (len > li.limit)
                    m_result.emplace_back(replaced_line { concat(" ORG   *+(",
                        // there seems be this strage artifical limit
                        std::min(len - li.limit, 1073676289ULL),
                        ")\n") });
                break;
            }
        }
        return true;
    };

    bool process_sql_type_operands(std::string_view operands, std::string_view label)
    {
        if (operands.size() < 2)
            return false;

        // keep the capture groups in sync
        static const auto xml_type = std::regex(
            "XML[ ]+AS[ ]+"
            "(?:"
            "(BINARY[ ]+LARGE[ ]+OBJECT|BLOB|CHARACTER[ ]+LARGE[ ]+OBJECT|CHAR[ ]+LARGE[ ]+OBJECT|CLOB|DBCLOB)"
            "[ ]+([[:digit:]]{1,9})([KMG])?"
            "|"
            "(BLOB_FILE|CLOB_FILE|DBCLOB_FILE)"
            ")"
            "(?: .*)?");
        static const auto lob_type = std::regex(
            "(?:"
            "(BINARY[ ]+LARGE[ ]+OBJECT|BLOB|CHARACTER[ ]+LARGE[ ]+OBJECT|CHAR[ ]+LARGE[ ]+OBJECT|CLOB|DBCLOB)"
            "[ ]+([[:digit:]]{1,9})([KMG])?"
            "|"
            "(BLOB_FILE|CLOB_FILE|DBCLOB_FILE|BLOB_LOCATOR|CLOB_LOCATOR|DBCLOB_LOCATOR)"
            ")"
            "(?: .*)?");

        static const auto table_like =
            std::regex("TABLE[ ]+LIKE[ ]+('(?:[^']|'')+'|(?:[^']|'')+)[ ]+AS[ ]+LOCATOR(?: .*)?");

        switch (operands[0])
        {
            case 'R':
                switch (operands[1])
                {
                    case 'E':
                        if (!consume_words(operands, { "RESULT_SET_LOCATOR", "VARYING" }, true))
                            break;
                        add_ds_line(label, "", "FL4");
                        return true;

                    case 'O':
                        if (!consume_words(operands, { "ROWID" }, true))
                            break;
                        add_ds_line(label, "", "H,CL40");
                        return true;
                }
                break;

            case 'T':
                if (!std::regex_match(operands.begin(), operands.end(), table_like))
                    break;
                add_ds_line(label, "", "FL4");
                return true;

            case 'X':
                return handle_lob(xml_type, label, operands);

            case 'B':
            case 'C':
            case 'D':
                return handle_lob(lob_type, label, operands);
        }
        return false;
    }

    void process_regular_line(std::string_view label, size_t first_line_skipped)
    {
        if (!label.empty())
            m_result.emplace_back(replaced_line { concat(label, " DS 0H\n") });

        m_result.emplace_back(replaced_line { "***$$$\n" });

        for (const auto& segment : m_logical_line.segments)
        {
            std::string this_line(segment.line);

            auto operand_part = segment.code;
            if (first_line_skipped)
            {
                const auto appended_line_size = segment.line.size();
                operand_part.remove_prefix(first_line_skipped);
                if (!label.empty())
                    this_line.replace(this_line.size() - appended_line_size,
                        label.size(),
                        label.size(),
                        ' '); // mask out any label-like characters
                this_line[this_line.size() - appended_line_size] = '*';

                first_line_skipped = 0;
            }
            this_line.append("\n");
            m_result.emplace_back(replaced_line { std::move(this_line) });
            m_operands.append(operand_part.substr(0, operand_part.find("--")));
        }
    }

    void process_sql_type_line(size_t first_line_skipped)
    {
        m_result.emplace_back(replaced_line { "***$$$\n" });
        m_result.emplace_back(replaced_line {
            concat("*", m_logical_line.segments.front().code.substr(0, lexing::default_ictl.end - 1), "\n") });

        for (const auto& segment : m_logical_line.segments)
        {
            m_operands.append(segment.code.substr(first_line_skipped));
            first_line_skipped = 0;
        }

        m_result.emplace_back(replaced_line { "***$$$\n" });
    }

    std::tuple<line_type, size_t, std::string_view> check_line(std::string_view input)
    {
        static constexpr std::tuple<line_type, size_t, std::string_view> ignore(line_type::ignore, 0, {});
        std::string_view line_preview = create_line_preview(input);

        if (ignore_line(line_preview))
            return ignore;

        size_t first_line_skipped = line_preview.size();
        std::string_view label = extract_label(line_preview);

        if (!remove_space(line_preview))
            return ignore;

        if (is_end(line_preview))
        {
            push_sql_working_storage();

            return ignore;
        }

        auto instruction = consume_instruction(line_preview);
        if (instruction == line_type::ignore)
            return ignore;

        if (!line_preview.empty())
            first_line_skipped = line_preview.data() - input.data();

        return { instruction, first_line_skipped, label };
    }

    static bool ord_char(unsigned char c) { return std::isalnum(c) || c == '_' || c == '@' || c == '$' || c == '#'; }

    static size_t count_arguments(std::string_view s)
    {
        size_t result = 0;

        while (!s.empty())
        {
            auto next = s.find_first_of(":'\"");
            if (next == std::string_view::npos)
                break;

            auto c = s[next];
            s.remove_prefix(next + 1);
            switch (c)
            {
                case ':':
                    ++result;
                    while (!s.empty() && s.front() == ' ') // skip optional spaces
                        s.remove_prefix(1);
                    while (!s.empty() && ord_char(s.front())) // skip host variable name
                        s.remove_prefix(1);
                    while (!s.empty() && s.front() == ' ') // skip spaces
                        s.remove_prefix(1);
                    if (!s.empty() && s.front() == ':') // null indicator?
                        s.remove_prefix(1);
                    break;

                case '\'':
                case '\"':
                    if (auto ending = s.find(c); ending == std::string_view::npos)
                        s = {};
                    else
                        s.remove_prefix(ending + 1);
                    break;
            }
        }

        return result;
    }

    void process_nonempty_line(
        size_t lineno, bool include_allowed, line_type instruction, size_t first_line_skipped, std::string_view label)
    {
        m_operands.clear();

        if (m_logical_line.continuation_error && m_diags)
            m_diags->add_diagnostic(diagnostic_op::error_DB001(range(position(lineno, 0))));

        switch (instruction)
        {
            case line_type::exec_sql:
                process_regular_line(label, first_line_skipped);
                if (sql_has_codegen(m_operands))
                    generate_sql_code_mock(count_arguments(m_operands));
                m_result.emplace_back(replaced_line { "***$$$\n" });
                break;

            case line_type::include:
                process_regular_line(label, first_line_skipped);

                if (std::string_view operands = trim_right(m_operands); include_allowed)
                    process_include(operands, lineno);
                else if (m_diags)
                    m_diags->add_diagnostic(diagnostic_op::error_DB003(range(position(lineno, 0)), operands));
                break;

            case line_type::sql_type:
                process_sql_type_line(first_line_skipped);
                // DB2 preprocessor exhibits strange behavior when SQL TYPE line is continued
                if (m_logical_line.segments.size() > 1 && m_diags)
                    m_diags->add_diagnostic(diagnostic_op::error_DB005(range(position(lineno, 0))));
                if (label.empty())
                    label = " "; // best matches the observed behavior
                if (!process_sql_type_operands(m_operands, label) && m_diags)
                    m_diags->add_diagnostic(diagnostic_op::error_DB004(range(position(lineno, 0))));
                break;
        }
    }

    static bool sql_has_codegen(std::string_view sql)
    {
        // handles only the most obvious cases (imprecisely)
        static const auto no_code_statements =
            std::regex("(?:DECLARE|WHENEVER|BEGIN[ ]+DECLARE[ ]+SECTION|END[ ]+DECLARE[ ]+SECTION)(?: .*)?",
                std::regex_constants::icase);
        return !std::regex_match(sql.begin(), sql.end(), no_code_statements);
    }
    void generate_sql_code_mock(size_t in_params)
    {
        // this function generates semi-realistic sql statement replacement code, because people do strange things...
        // <arguments> output parameters
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

    void generate_replacement(line_iterator it, line_iterator end, bool include_allowed)
    {
        bool skip_continuation = false;
        while (it != end)
        {
            const auto text = it->text();
            if (skip_continuation)
            {
                m_result.emplace_back(*it++);
                skip_continuation = is_continued(text);
                continue;
            }
            auto [instruction, first_line_skipped, label] = check_line(text);
            if (instruction == line_type::ignore)
            {
                m_result.emplace_back(*it++);
                skip_continuation = is_continued(text);
                continue;
            }

            m_source_translated = true;

            m_logical_line.clear();

            size_t lineno = it->lineno().value_or(0); // TODO: needs to be addressed for chained preprocessors

            it = extract_nonempty_logical_line(m_logical_line, it, end, lexing::default_ictl);

            process_nonempty_line(lineno, include_allowed, instruction, first_line_skipped, label);
        }
    }

    // Inherited via preprocessor
    document generate_replacement(document doc) override
    {
        m_source_translated = false;
        m_result.clear();
        m_result.reserve(doc.size());

        auto it = doc.begin();
        const auto end = doc.end();

        skip_process(it, end);
        // ignores ICTL
        inject_SQLSECT();

        generate_replacement(it, end, true);

        if (m_source_translated || !m_conditional)
            return document(std::move(m_result));
        else
            return doc;
    }

public:
    db2_preprocessor(const db2_preprocessor_options& opts, library_fetcher libs, diagnostic_op_consumer* diags)
        : m_version(opts.version)
        , m_conditional(opts.conditional)
        , m_libs(std::move(libs))
        , m_diags(diags)
    {}
};
} // namespace

std::unique_ptr<preprocessor> preprocessor::create(
    const db2_preprocessor_options& opts, library_fetcher libs, diagnostic_op_consumer* diags)
{
    return std::make_unique<db2_preprocessor>(opts, std::move(libs), diags);
}

} // namespace hlasm_plugin::parser_library::processing
