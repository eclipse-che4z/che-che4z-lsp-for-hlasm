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

#include "lexing/logical_line.h"
#include "preprocessor.h"
#include "preprocessor_options.h"
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
class db2_preprocessor : public preprocessor
{
    const char* m_last_position = nullptr;
    lexing::logical_line m_logical_line;
    std::string m_operands;
    library_fetcher m_libs;
    diag_reporter m_diags;
    std::string m_buffer;

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

    void push_sql_working_storage()
    {
        m_buffer.append("***$$$ SQL WORKING STORAGE                      \n"
                        "SQLDSIZ  DC    A(SQLDLEN) SQLDSECT SIZE         \n"
                        "SQLDSECT DSECT                                  \n"
                        "SQLTEMP  DS    CL128     TEMPLATE               \n"
                        "DSNTEMP  DS    F         INT SCROLL VALUE       \n"
                        "DSNTMP2  DS    PL16      DEC SCROLL VALUE       \n"
                        "DSNNROWS DS    F         MULTI-ROW N-ROWS VALUE \n"
                        "DSNNTYPE DS    H         MULTI-ROW N-ROWS TYPE  \n"
                        "DSNNLEN  DS    H         MULTI-ROW N-ROWS LENGTH\n"
                        "DSNPARMS DS    4F        DSNHMLTR PARM LIST     \n"
                        "DSNPNM   DS    CL386     PROCEDURE NAME         \n"
                        "DSNCNM   DS    CL128     CURSOR NAME            \n"
                        "SQL_FILE_READ      EQU 2                        \n"
                        "SQL_FILE_CREATE    EQU 8                        \n"
                        "SQL_FILE_OVERWRITE EQU 16                       \n"
                        "SQL_FILE_APPEND    EQU 32                       \n"
                        "         DS    0D                               \n"
                        "SQLPLIST DS    F                                \n"
                        "SQLPLLEN DS    H         PLIST LENGTH           \n"
                        "SQLFLAGS DS    XL2       FLAGS                  \n"
                        "SQLCTYPE DS    H         CALL-TYPE              \n"
                        "SQLPROGN DS    CL8       PROGRAM NAME           \n"
                        "SQLTIMES DS    CL8       TIMESTAMP              \n"
                        "SQLSECTN DS    H         SECTION                \n"
                        "SQLCODEP DS    A         CODE POINTER           \n"
                        "SQLVPARM DS    A         VPARAM POINTER         \n"
                        "SQLAPARM DS    A         AUX PARAM PTR          \n"
                        "SQLSTNM7 DS    H         PRE_V8 STATEMENT NUMBER\n"
                        "SQLSTYPE DS    H         STATEMENT TYPE         \n"
                        "SQLSTNUM DS    F         STATEMENT NUMBER       \n"
                        "SQLFLAG2 DS    H         internal flags         \n"
                        "SQLRSRVD DS    CL18      RESERVED               \n"
                        "SQLPVARS DS    CL8,F,2H,0CL44                   \n"
                        "SQLAVARS DS    CL8,F,2H,0CL44                   \n"
                        "         DS    0D                               \n"
                        "SQLDLEN  EQU   *-SQLDSECT                       \n");
    }

    void inject_SQLCA()
    {
        m_buffer.append("***$$$ SQLCA                          \n"
                        "SQLCA    DS    0F                     \n"
                        "SQLCAID  DS    CL8      ID            \n"
                        "SQLCABC  DS    F        BYTE COUNT    \n"
                        "SQLCODE  DS    F        RETURN CODE   \n"
                        "SQLERRM  DS    H,CL70   ERR MSG PARMS \n"
                        "SQLERRP  DS    CL8      IMPL-DEPENDENT\n"
                        "SQLERRD  DS    6F                     \n"
                        "SQLWARN  DS    0C       WARNING FLAGS \n"
                        "SQLWARN0 DS    C'W' IF ANY            \n"
                        "SQLWARN1 DS    C'W' = WARNING         \n"
                        "SQLWARN2 DS    C'W' = WARNING         \n"
                        "SQLWARN3 DS    C'W' = WARNING         \n"
                        "SQLWARN4 DS    C'W' = WARNING         \n"
                        "SQLWARN5 DS    C'W' = WARNING         \n"
                        "SQLWARN6 DS    C'W' = WARNING         \n"
                        "SQLWARN7 DS    C'W' = WARNING         \n"
                        "SQLEXT   DS    0CL8                   \n"
                        "SQLWARN8 DS    C                      \n"
                        "SQLWARN9 DS    C                      \n"
                        "SQLWARNA DS    C                      \n"
                        "SQLSTATE DS    CL5                    \n"
                        "***$$$\n");
    }
    void inject_SQLDA()
    {
        m_buffer.append("***$$$ SQLDA                                            \n"
                        "SQLTRIPL EQU    C'3'                                    \n"
                        "SQLDOUBL EQU    C'2'                                    \n"
                        "SQLSINGL EQU    C' '                                    \n"
                        "*                                                       \n"
                        "         SQLSECT SAVE                                   \n"
                        "*                                                       \n"
                        "SQLDA    DSECT                                          \n"
                        "SQLDAID  DS    CL8      ID                              \n"
                        "SQLDABC  DS    F        BYTE COUNT                      \n"
                        "SQLN     DS    H        COUNT SQLVAR/SQLVAR2 ENTRIES    \n"
                        "SQLD     DS    H        COUNT VARS (TWICE IF USING BOTH)\n"
                        "*                                                       \n"
                        "SQLVAR   DS    0F       BEGIN VARS                      \n"
                        "SQLVARN  DSECT ,        NTH VARIABLE                    \n"
                        "SQLTYPE  DS    H        DATA TYPE CODE                  \n"
                        "SQLLEN   DS    0H       LENGTH                          \n"
                        "SQLPRCSN DS    X        DEC PRECISION                   \n"
                        "SQLSCALE DS    X        DEC SCALE                       \n"
                        "SQLDATA  DS    A        ADDR OF VAR                     \n"
                        "SQLIND   DS    A        ADDR OF IND                     \n"
                        "SQLNAME  DS    H,CL30   DESCRIBE NAME                   \n"
                        "SQLVSIZ  EQU   *-SQLDATA                                \n"
                        "SQLSIZV  EQU   *-SQLVARN                                \n"
                        "*                                                       \n"
                        "SQLDA    DSECT                                          \n"
                        "SQLVAR2  DS     0F      BEGIN EXTENDED FIELDS OF VARS   \n"
                        "SQLVAR2N DSECT  ,       EXTENDED FIELDS OF NTH VARIABLE \n"
                        "SQLLONGL DS     F       LENGTH                          \n"
                        "SQLRSVDL DS     F       RESERVED                        \n"
                        "SQLDATAL DS     A       ADDR OF LENGTH IN BYTES         \n"
                        "SQLTNAME DS     H,CL30  DESCRIBE NAME                   \n"
                        "*                                                       \n"
                        "         SQLSECT RESTORE                                \n"
                        "***$$$\n");
    }
    void inject_SQLSECT()
    {
        m_buffer.append("         MACRO                          \n"
                        "         SQLSECT &TYPE                  \n"
                        "         GBLC  &SQLSECT                 \n"
                        "         AIF ('&TYPE' EQ 'RESTORE').REST\n"
                        "&SQLSECT SETC  '&SYSECT'                \n"
                        "         MEXIT                          \n"
                        ".REST    ANOP                           \n"
                        "&SQLSECT CSECT                          \n"
                        "         MEND                           \n");
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
        m_buffer.append("***$$$\n");

        std::optional<std::string> include_text;
        if (m_libs)
            include_text = m_libs(operands);
        if (!include_text.has_value())
        {
            if (m_diags)
                m_diags(diagnostic_op::error_P0002(range(position(lineno, 0)), operands));
            return;
        }

        std::string_view include = include_text.value();

        while (!include.empty())
        {
            if (fill_buffer(include, lineno, false) > 0)
                continue;
            while (true)
            {
                const auto text = lexing::extract_line(include).first;
                m_buffer.append(text);
                m_buffer.append("\n");
                if (text.size() <= lexing::default_ictl_copy.continuation
                    || text[lexing::default_ictl_copy.continuation] == ' ')
                    break;
            }
        }
    }

    static bool consume_exec_sql(std::string_view& l)
    {
        if (!consume(l, "EXEC"))
            return false;
        if (!remove_space(l))
            return false;
        if (!consume(l, "SQL"))
            return false;
        return remove_space(l);
    }

    static bool is_end(std::string_view s)
    {
        if (!consume(s, "END"))
            return false;

        if (s.empty() || s.front() == ' ')
            return true;

        return false;
    }

    static bool consume_include(std::string_view& s)
    {
        if (!consume(s, "INCLUDE"))
            return false;

        return remove_space(s);
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

    /* returns number of consumed lines */
    size_t fill_buffer(std::string_view& input, size_t lineno, bool include_allowed)
    {
        using namespace std::literals;

        std::string_view line_preview = create_line_preview(input);

        if (ignore_line(line_preview))
            return 0;

        size_t first_line_skipped = line_preview.size();
        std::string_view label = extract_label(line_preview);

        if (!remove_space(line_preview))
            return 0;

        if (is_end(line_preview))
        {
            push_sql_working_storage();

            return 0;
        }

        if (!consume_exec_sql(line_preview))
            return 0;
        if (!line_preview.empty())
            first_line_skipped = line_preview.data() - input.data();

        // now we have a valid EXEC SQL line

        m_operands.clear();
        m_logical_line.clear();

        bool extracted = lexing::extract_logical_line(m_logical_line, input, lexing::default_ictl);
        assert(extracted);

        if (m_logical_line.continuation_error && m_diags)
            m_diags(diagnostic_op::error_P0001(range(position(lineno, 0))));

        if (!label.empty())
            m_buffer.append(label).append(" DS 0H\n");

        m_buffer.append("***$$$\n");

        for (const auto& segment : m_logical_line.segments)
        {
            m_buffer.append(segment.line);

            auto operand_part = segment.code;
            if (first_line_skipped)
            {
                const auto appended_line_size = segment.line.size();
                operand_part.remove_prefix(first_line_skipped);
                if (!label.empty())
                    m_buffer.replace(m_buffer.size() - appended_line_size,
                        label.size(),
                        label.size(),
                        ' '); // mask out any label-like characters
                m_buffer[m_buffer.size() - appended_line_size] = '*';

                first_line_skipped = 0;
            }
            m_buffer.append("\n");
            m_operands.append(operand_part);
        }

        std::string_view operands = m_operands;
        bool is_include = false;
        if (consume_include(operands))
        {
            operands = trim_right(operands);

            if (include_allowed)
                is_include = true;
            else if (m_diags)
                m_diags(diagnostic_op::error_P0003(range(position(lineno, 0)), operands));
        }

        if (is_include)
            process_include(operands, lineno);
        else
            m_buffer.append("***$$$\n");

        return m_logical_line.segments.size();
    }

    // Inherited via preprocessor
    std::optional<std::string> generate_replacement(std::string_view& input, size_t& lineno) override
    {
        if (input.data() == m_last_position)
            return std::nullopt;

        m_buffer.clear();
        if (std::exchange(m_last_position, input.data()) == nullptr)
        {
            // injected right after ICTL or *PROCESS
            inject_SQLSECT();
        }

        lineno += fill_buffer(input, lineno, true);
        if (m_buffer.size())
            return m_buffer;
        else
            return std::nullopt;
    }

public:
    db2_preprocessor(library_fetcher libs, diag_reporter diags)
        : m_libs(std::move(libs))
        , m_diags(std::move(diags))
    {}
};
} // namespace

std::unique_ptr<preprocessor> preprocessor::create(
    const db2_preprocessor_options&, library_fetcher libs, diag_reporter diags)
{
    return std::make_unique<db2_preprocessor>(std::move(libs), std::move(diags));
}
} // namespace hlasm_plugin::parser_library::processing
