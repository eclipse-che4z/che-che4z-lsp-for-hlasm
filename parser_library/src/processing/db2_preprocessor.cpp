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

    static bool remove_space(std::string_view& s)
    {
        if (s.empty() || s.front() != ' ')
            return false;
        const auto non_space = s.find_first_not_of(' ');

        if (non_space == std::string_view::npos)
            return false;

        s = s.substr(non_space);
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

    void push_sql_working_storage(std::deque<std::string>& queue)
    {
        queue.emplace_back("***$$$ SQL WORKING STORAGE                      ");
        queue.emplace_back("SQLDSIZ  DC    A(SQLDLEN) SQLDSECT SIZE         ");
        queue.emplace_back("SQLDSECT DSECT                                  ");
        queue.emplace_back("SQLTEMP  DS    CL128     TEMPLATE               ");
        queue.emplace_back("DSNTEMP  DS    F         INT SCROLL VALUE       ");
        queue.emplace_back("DSNTMP2  DS    PL16      DEC SCROLL VALUE       ");
        queue.emplace_back("DSNNROWS DS    F         MULTI-ROW N-ROWS VALUE ");
        queue.emplace_back("DSNNTYPE DS    H         MULTI-ROW N-ROWS TYPE  ");
        queue.emplace_back("DSNNLEN  DS    H         MULTI-ROW N-ROWS LENGTH");
        queue.emplace_back("DSNPARMS DS    4F        DSNHMLTR PARM LIST     ");
        queue.emplace_back("DSNPNM   DS    CL386     PROCEDURE NAME         ");
        queue.emplace_back("DSNCNM   DS    CL128     CURSOR NAME            ");
        queue.emplace_back("SQL_FILE_READ      EQU 2                        ");
        queue.emplace_back("SQL_FILE_CREATE    EQU 8                        ");
        queue.emplace_back("SQL_FILE_OVERWRITE EQU 16                       ");
        queue.emplace_back("SQL_FILE_APPEND    EQU 32                       ");
        queue.emplace_back("         DS    0D                               ");
        queue.emplace_back("SQLPLIST DS    F                                ");
        queue.emplace_back("SQLPLLEN DS    H         PLIST LENGTH           ");
        queue.emplace_back("SQLFLAGS DS    XL2       FLAGS                  ");
        queue.emplace_back("SQLCTYPE DS    H         CALL-TYPE              ");
        queue.emplace_back("SQLPROGN DS    CL8       PROGRAM NAME           ");
        queue.emplace_back("SQLTIMES DS    CL8       TIMESTAMP              ");
        queue.emplace_back("SQLSECTN DS    H         SECTION                ");
        queue.emplace_back("SQLCODEP DS    A         CODE POINTER           ");
        queue.emplace_back("SQLVPARM DS    A         VPARAM POINTER         ");
        queue.emplace_back("SQLAPARM DS    A         AUX PARAM PTR          ");
        queue.emplace_back("SQLSTNM7 DS    H         PRE_V8 STATEMENT NUMBER");
        queue.emplace_back("SQLSTYPE DS    H         STATEMENT TYPE         ");
        queue.emplace_back("SQLSTNUM DS    F         STATEMENT NUMBER       ");
        queue.emplace_back("SQLFLAG2 DS    H         internal flags         ");
        queue.emplace_back("SQLRSRVD DS    CL18      RESERVED               ");
        queue.emplace_back("SQLPVARS DS    CL8,F,2H,0CL44                   ");
        queue.emplace_back("SQLAVARS DS    CL8,F,2H,0CL44                   ");
        queue.emplace_back("         DS    0D                               ");
        queue.emplace_back("SQLDLEN  EQU   *-SQLDSECT                       ");
    }

    void inject_SQLCA(std::deque<std::string>& queue)
    {
        queue.emplace_back("***$$$ SQLCA                          ");
        queue.emplace_back("SQLCA    DS    0F                     ");
        queue.emplace_back("SQLCAID  DS    CL8      ID            ");
        queue.emplace_back("SQLCABC  DS    F        BYTE COUNT    ");
        queue.emplace_back("SQLCODE  DS    F        RETURN CODE   ");
        queue.emplace_back("SQLERRM  DS    H,CL70   ERR MSG PARMS ");
        queue.emplace_back("SQLERRP  DS    CL8      IMPL-DEPENDENT");
        queue.emplace_back("SQLERRD  DS    6F                     ");
        queue.emplace_back("SQLWARN  DS    0C       WARNING FLAGS ");
        queue.emplace_back("SQLWARN0 DS    C'W' IF ANY            ");
        queue.emplace_back("SQLWARN1 DS    C'W' = WARNING         ");
        queue.emplace_back("SQLWARN2 DS    C'W' = WARNING         ");
        queue.emplace_back("SQLWARN3 DS    C'W' = WARNING         ");
        queue.emplace_back("SQLWARN4 DS    C'W' = WARNING         ");
        queue.emplace_back("SQLWARN5 DS    C'W' = WARNING         ");
        queue.emplace_back("SQLWARN6 DS    C'W' = WARNING         ");
        queue.emplace_back("SQLWARN7 DS    C'W' = WARNING         ");
        queue.emplace_back("SQLEXT   DS    0CL8                   ");
        queue.emplace_back("SQLWARN8 DS    C                      ");
        queue.emplace_back("SQLWARN9 DS    C                      ");
        queue.emplace_back("SQLWARNA DS    C                      ");
        queue.emplace_back("SQLSTATE DS    CL5                    ");
        queue.emplace_back("***$$$");
    }
    void inject_SQLDA(std::deque<std::string>& queue)
    {
        queue.emplace_back("***$$$ SQLDA                                            ");
        queue.emplace_back("SQLTRIPL EQU    C'3'                                    ");
        queue.emplace_back("SQLDOUBL EQU    C'2'                                    ");
        queue.emplace_back("SQLSINGL EQU    C' '                                    ");
        queue.emplace_back("*                                                       ");
        queue.emplace_back("         SQLSECT SAVE                                   ");
        queue.emplace_back("*                                                       ");
        queue.emplace_back("SQLDA    DSECT                                          ");
        queue.emplace_back("SQLDAID  DS    CL8      ID                              ");
        queue.emplace_back("SQLDABC  DS    F        BYTE COUNT                      ");
        queue.emplace_back("SQLN     DS    H        COUNT SQLVAR/SQLVAR2 ENTRIES    ");
        queue.emplace_back("SQLD     DS    H        COUNT VARS (TWICE IF USING BOTH)");
        queue.emplace_back("*                                                       ");
        queue.emplace_back("SQLVAR   DS    0F       BEGIN VARS                      ");
        queue.emplace_back("SQLVARN  DSECT ,        NTH VARIABLE                    ");
        queue.emplace_back("SQLTYPE  DS    H        DATA TYPE CODE                  ");
        queue.emplace_back("SQLLEN   DS    0H       LENGTH                          ");
        queue.emplace_back("SQLPRCSN DS    X        DEC PRECISION                   ");
        queue.emplace_back("SQLSCALE DS    X        DEC SCALE                       ");
        queue.emplace_back("SQLDATA  DS    A        ADDR OF VAR                     ");
        queue.emplace_back("SQLIND   DS    A        ADDR OF IND                     ");
        queue.emplace_back("SQLNAME  DS    H,CL30   DESCRIBE NAME                   ");
        queue.emplace_back("SQLVSIZ  EQU   *-SQLDATA                                ");
        queue.emplace_back("SQLSIZV  EQU   *-SQLVARN                                ");
        queue.emplace_back("*                                                       ");
        queue.emplace_back("SQLDA    DSECT                                          ");
        queue.emplace_back("SQLVAR2  DS     0F      BEGIN EXTENDED FIELDS OF VARS   ");
        queue.emplace_back("SQLVAR2N DSECT  ,       EXTENDED FIELDS OF NTH VARIABLE ");
        queue.emplace_back("SQLLONGL DS     F       LENGTH                          ");
        queue.emplace_back("SQLRSVDL DS     F       RESERVED                        ");
        queue.emplace_back("SQLDATAL DS     A       ADDR OF LENGTH IN BYTES         ");
        queue.emplace_back("SQLTNAME DS     H,CL30  DESCRIBE NAME                   ");
        queue.emplace_back("*                                                       ");
        queue.emplace_back("         SQLSECT RESTORE                                ");
        queue.emplace_back("***$$$");
    }
    void inject_SQLSECT(std::deque<std::string>& queue)
    {
        queue.emplace_back("         MACRO                          ");
        queue.emplace_back("         SQLSECT &TYPE                  ");
        queue.emplace_back("         GBLC  &SQLSECT                 ");
        queue.emplace_back("         AIF ('&TYPE' EQ 'RESTORE').REST");
        queue.emplace_back("&SQLSECT SETC  '&SYSECT'                ");
        queue.emplace_back("         MEXIT                          ");
        queue.emplace_back(".REST    ANOP                           ");
        queue.emplace_back("&SQLSECT CSECT                          ");
        queue.emplace_back("         MEND                           ");
    }

    void process_include(std::string_view operands, size_t lineno, std::deque<std::string>& queue)
    {
        if (operands == "SQLCA")
        {
            inject_SQLCA(queue);
            return;
        }
        if (operands == "SQLDA")
        {
            inject_SQLDA(queue);
            return;
        }
        queue.emplace_back("***$$$");

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
            if (fill_buffer(include, lineno, queue, false).removed_lines_from_input)
                continue;
            while (true)
            {
                const auto text = lexing::extract_line(include).first;
                queue.emplace_back(text);
                if (text.size() <= lexing::default_ictl_copy.continuation
                    || text[lexing::default_ictl_copy.continuation] == ' ')
                    break;
            }
        }
    }

    static bool consume_exec_sql(std::string_view& l)
    {
        using namespace std::literals;
        constexpr const auto EXEC_literal = "EXEC"sv;
        constexpr const auto SQL_literal = "SQL"sv;

        const char* start = l.data();

        if (!consume(l, EXEC_literal))
            return false;
        if (!remove_space(l))
            return false;
        if (!consume(l, SQL_literal))
            return false;
        return remove_space(l);
    }

    static bool is_end(std::string_view s)
    {
        using namespace std::literals;
        constexpr const auto END_literal = "END"sv;

        if (!consume(s, END_literal))
            return false;

        if (s.empty() || s.front() == ' ')
            return true;

        return false;
    }

    static bool consume_include(std::string_view& s)
    {
        using namespace std::literals;

        constexpr const auto INCLUDE_literal = "INCLUDE"sv;

        if (consume(s, INCLUDE_literal))
        {
            if (remove_space(s))
                return true;
        }
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

    struct fill_buffer_result
    {
        size_t removed_lines_from_input = 0;
        bool lines_inserted = false;
    };

    /* returns number of consumed lines or -1 when nothing was inserted */
    fill_buffer_result fill_buffer(
        std::string_view& input, size_t lineno, std::deque<std::string>& queue, bool include_allowed)
    {
        using namespace std::literals;

        std::string_view line_preview = create_line_preview(input);

        if (ignore_line(line_preview))
            return {};

        std::string_view label = extract_label(line_preview);

        if (!remove_space(line_preview))
            return {};

        if (is_end(line_preview))
        {
            push_sql_working_storage(queue);

            return { 0, true };
        }

        if (!consume_exec_sql(line_preview))
            return {};
        auto first_line_skipped = line_preview.data() - input.data();

        // now we have a valid EXEC SQL line

        m_operands.clear();
        m_logical_line.clear();

        bool extracted = lexing::extract_logical_line(m_logical_line, input, lexing::default_ictl);
        assert(extracted);

        if (m_logical_line.continuation_error && m_diags)
            m_diags(diagnostic_op::error_P0001(range(position(lineno, 0))));

        if (!label.empty())
            queue.emplace_back(label) += " DS 0H";

        queue.emplace_back("***$$$");

        for (const auto& segment : m_logical_line.segments)
        {
            auto& l = queue.emplace_back(segment.line);

            auto operand_part = segment.code;
            if (first_line_skipped)
            {
                operand_part.remove_prefix(first_line_skipped);
                first_line_skipped = 0;
                if (!l.empty())
                    l.replace(0, label.size(), label.size(), ' '); // mask out any label-like characters
                l[0] = '*';
            }
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
            process_include(operands, lineno, queue);
        else
            queue.emplace_back("***$$$");

        return { m_logical_line.segments.size(), true };
    }

    // Inherited via preprocessor
    virtual bool fill_buffer(std::string_view& input, size_t& lineno, std::deque<std::string>& queue) override
    {
        bool lines_inserted = false;
        if (input.data() == m_last_position)
            return false;

        if (std::exchange(m_last_position, input.data()) == nullptr)
        {
            // injected right after ICTL or *PROCESS
            inject_SQLSECT(queue);
            lines_inserted = true;
        }

        const auto result = fill_buffer(input, lineno, queue, true);
        lineno += result.removed_lines_from_input;
        return lines_inserted || result.lines_inserted;
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
