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

namespace hlasm_plugin::parser_library::processing {
namespace {
class db2_preprocessor : public preprocessor
{
    const char* m_last_position = nullptr;
    lexing::logical_line m_logical_line;
    std::string m_operands;
    library_fetcher m_libs;
    diag_reporter m_diags;

    static bool remove_non_space(std::string_view& s)
    {
        if (s.empty() || s.front() == ' ')
            return false;
        const auto space = s.find(' ');

        if (space == std::string_view::npos)
            return false;

        s = s.substr(space);
        return true;
    }

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
        queue.push_back("***$$$ SQL WORKING STORAGE                      ");
        queue.push_back("SQLDSIZ  DC    A(SQLDLEN) SQLDSECT SIZE         ");
        queue.push_back("SQLDSECT DSECT                                  ");
        queue.push_back("SQLTEMP  DS    CL128     TEMPLATE               ");
        queue.push_back("DSNTEMP  DS    F         INT SCROLL VALUE       ");
        queue.push_back("DSNTMP2  DS    PL16      DEC SCROLL VALUE       ");
        queue.push_back("DSNNROWS DS    F         MULTI-ROW N-ROWS VALUE ");
        queue.push_back("DSNNTYPE DS    H         MULTI-ROW N-ROWS TYPE  ");
        queue.push_back("DSNNLEN  DS    H         MULTI-ROW N-ROWS LENGTH");
        queue.push_back("DSNPARMS DS    4F        DSNHMLTR PARM LIST     ");
        queue.push_back("DSNPNM   DS    CL386     PROCEDURE NAME         ");
        queue.push_back("DSNCNM   DS    CL128     CURSOR NAME            ");
        queue.push_back("SQL_FILE_READ      EQU 2                        ");
        queue.push_back("SQL_FILE_CREATE    EQU 8                        ");
        queue.push_back("SQL_FILE_OVERWRITE EQU 16                       ");
        queue.push_back("SQL_FILE_APPEND    EQU 32                       ");
        queue.push_back("         DS    0D                               ");
        queue.push_back("SQLPLIST DS    F                                ");
        queue.push_back("SQLPLLEN DS    H         PLIST LENGTH           ");
        queue.push_back("SQLFLAGS DS    XL2       FLAGS                  ");
        queue.push_back("SQLCTYPE DS    H         CALL-TYPE              ");
        queue.push_back("SQLPROGN DS    CL8       PROGRAM NAME           ");
        queue.push_back("SQLTIMES DS    CL8       TIMESTAMP              ");
        queue.push_back("SQLSECTN DS    H         SECTION                ");
        queue.push_back("SQLCODEP DS    A         CODE POINTER           ");
        queue.push_back("SQLVPARM DS    A         VPARAM POINTER         ");
        queue.push_back("SQLAPARM DS    A         AUX PARAM PTR          ");
        queue.push_back("SQLSTNM7 DS    H         PRE_V8 STATEMENT NUMBER");
        queue.push_back("SQLSTYPE DS    H         STATEMENT TYPE         ");
        queue.push_back("SQLSTNUM DS    F         STATEMENT NUMBER       ");
        queue.push_back("SQLFLAG2 DS    H         internal flags         ");
        queue.push_back("SQLRSRVD DS    CL18      RESERVED               ");
        queue.push_back("SQLPVARS DS    CL8,F,2H,0CL44                   ");
        queue.push_back("SQLAVARS DS    CL8,F,2H,0CL44                   ");
        queue.push_back("         DS    0D                               ");
        queue.push_back("SQLDLEN  EQU   *-SQLDSECT                       ");
    }

    void inject_SQLCA(std::deque<std::string>& queue)
    {
        queue.push_back("***$$$ SQLCA                          ");
        queue.push_back("SQLCA    DS    0F                     ");
        queue.push_back("SQLCAID  DS    CL8      ID            ");
        queue.push_back("SQLCABC  DS    F        BYTE COUNT    ");
        queue.push_back("SQLCODE  DS    F        RETURN CODE   ");
        queue.push_back("SQLERRM  DS    H,CL70   ERR MSG PARMS ");
        queue.push_back("SQLERRP  DS    CL8      IMPL-DEPENDENT");
        queue.push_back("SQLERRD  DS    6F                     ");
        queue.push_back("SQLWARN  DS    0C       WARNING FLAGS ");
        queue.push_back("SQLWARN0 DS    C'W' IF ANY            ");
        queue.push_back("SQLWARN1 DS    C'W' = WARNING         ");
        queue.push_back("SQLWARN2 DS    C'W' = WARNING         ");
        queue.push_back("SQLWARN3 DS    C'W' = WARNING         ");
        queue.push_back("SQLWARN4 DS    C'W' = WARNING         ");
        queue.push_back("SQLWARN5 DS    C'W' = WARNING         ");
        queue.push_back("SQLWARN6 DS    C'W' = WARNING         ");
        queue.push_back("SQLWARN7 DS    C'W' = WARNING         ");
        queue.push_back("SQLEXT   DS    0CL8                   ");
        queue.push_back("SQLWARN8 DS    C                      ");
        queue.push_back("SQLWARN9 DS    C                      ");
        queue.push_back("SQLWARNA DS    C                      ");
        queue.push_back("SQLSTATE DS    CL5                    ");
        queue.push_back("***$$$");
    }
    void inject_SQLDA(std::deque<std::string>& queue)
    {
        queue.push_back("***$$$ SQLDA                                            ");
        queue.push_back("SQLTRIPL EQU    C'3'                                    ");
        queue.push_back("SQLDOUBL EQU    C'2'                                    ");
        queue.push_back("SQLSINGL EQU    C' '                                    ");
        queue.push_back("*                                                       ");
        queue.push_back("         SQLSECT SAVE                                   ");
        queue.push_back("*                                                       ");
        queue.push_back("SQLDA    DSECT                                          ");
        queue.push_back("SQLDAID  DS    CL8      ID                              ");
        queue.push_back("SQLDABC  DS    F        BYTE COUNT                      ");
        queue.push_back("SQLN     DS    H        COUNT SQLVAR/SQLVAR2 ENTRIES    ");
        queue.push_back("SQLD     DS    H        COUNT VARS (TWICE IF USING BOTH)");
        queue.push_back("*                                                       ");
        queue.push_back("SQLVAR   DS    0F       BEGIN VARS                      ");
        queue.push_back("SQLVARN  DSECT ,        NTH VARIABLE                    ");
        queue.push_back("SQLTYPE  DS    H        DATA TYPE CODE                  ");
        queue.push_back("SQLLEN   DS    0H       LENGTH                          ");
        queue.push_back("SQLPRCSN DS    X        DEC PRECISION                   ");
        queue.push_back("SQLSCALE DS    X        DEC SCALE                       ");
        queue.push_back("SQLDATA  DS    A        ADDR OF VAR                     ");
        queue.push_back("SQLIND   DS    A        ADDR OF IND                     ");
        queue.push_back("SQLNAME  DS    H,CL30   DESCRIBE NAME                   ");
        queue.push_back("SQLVSIZ  EQU   *-SQLDATA                                ");
        queue.push_back("SQLSIZV  EQU   *-SQLVARN                                ");
        queue.push_back("*                                                       ");
        queue.push_back("SQLDA    DSECT                                          ");
        queue.push_back("SQLVAR2  DS     0F      BEGIN EXTENDED FIELDS OF VARS   ");
        queue.push_back("SQLVAR2N DSECT  ,       EXTENDED FIELDS OF NTH VARIABLE ");
        queue.push_back("SQLLONGL DS     F       LENGTH                          ");
        queue.push_back("SQLRSVDL DS     F       RESERVED                        ");
        queue.push_back("SQLDATAL DS     A       ADDR OF LENGTH IN BYTES         ");
        queue.push_back("SQLTNAME DS     H,CL30  DESCRIBE NAME                   ");
        queue.push_back("*                                                       ");
        queue.push_back("         SQLSECT RESTORE                                ");
        queue.push_back("***$$$");
    }
    void inject_SQLSECT(std::deque<std::string>& queue)
    {
        queue.push_back("         MACRO                          ");
        queue.push_back("         SQLSECT &TYPE                  ");
        queue.push_back("         GBLC  &SQLSECT                 ");
        queue.push_back("         AIF ('&TYPE' EQ 'RESTORE').REST");
        queue.push_back("&SQLSECT SETC  '&SYSECT'                ");
        queue.push_back("         MEXIT                          ");
        queue.push_back(".REST    ANOP                           ");
        queue.push_back("&SQLSECT CSECT                          ");
        queue.push_back("         MEND                           ");
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
        queue.push_back("***$$$");

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
            if (fill_buffer(include, lineno, queue, false))
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

    virtual bool fill_buffer(
        std::string_view& input, size_t lineno, std::deque<std::string>& queue, bool include_allowed)
    {
        using namespace std::literals;
        if (input.data() == m_last_position)
            return false;

        bool injected = false;
        if (std::exchange(m_last_position, input.data()) == nullptr)
        {
            // injected right after ICTL or *PROCESS
            inject_SQLSECT(queue);
            injected = true;
        }

        if (input.size() < lexing::default_ictl.begin - 1)
            return injected;

        std::string_view line_preview =
            input.substr(lexing::default_ictl.begin - 1, lexing::default_ictl.end - (lexing::default_ictl.begin - 1));

        const auto rn = line_preview.find_first_of("\r\n"sv);
        if (rn != std::string_view::npos)
            line_preview = line_preview.substr(0, rn);

        if (line_preview.empty())
            return injected;

        std::string label;
        if (line_preview.front() != ' ')
        {
            if (!remove_non_space(line_preview))
                return injected;
            label = std::string_view(input.data(), line_preview.data() - input.data());
        }
        if (!remove_space(line_preview))
            return injected;

        constexpr const auto END_literal = "END"sv;
        constexpr const auto EXEC_literal = "EXEC"sv;
        constexpr const auto SQL_literal = "SQL"sv;
        constexpr const auto INCLUDE_literal = "INCLUDE"sv;

        if (consume(line_preview, END_literal))
        {
            if (line_preview.empty() || line_preview.front() == ' ')
            {
                push_sql_working_storage(queue);
                return true;
            }
            return injected;
        }

        if (!consume(line_preview, EXEC_literal))
            return injected;
        if (!remove_space(line_preview))
            return injected;
        if (!consume(line_preview, SQL_literal))
            return injected;
        if (line_preview.empty() || line_preview.front() != ' ')
            return injected;
        remove_space(line_preview);
        const auto skipped = line_preview.data() - input.data();

        // now we have a valid EXEC SQL line

        m_operands.clear();
        m_logical_line.clear();
        bool extracted = lexing::extract_logical_line(m_logical_line, input, lexing::default_ictl);
        assert(extracted);

        if (m_logical_line.continuation_error && m_diags)
            m_diags(diagnostic_op::error_P0001(range(position(lineno, 0))));

        if (!label.empty())
            queue.emplace_back(label) += " DS 0H";

        queue.push_back("***$$$");
        bool first_line = true;
        for (const auto& segment : m_logical_line.segments)
        {
            auto& l = queue.emplace_back();
            if (first_line)
            {
                first_line = false;
                l.append(segment.code);
                if (!label.empty())
                    l.replace(0, label.size(), label.size(), ' '); // mask out any label-like characters
                l[0] = '*';
                m_operands.append(segment.code.substr(skipped));
            }
            else
            {
                l.append(lexing::default_ictl.continuation - lexing::default_ictl.begin,
                    segment.continuation_error ? 'X' : ' ');
                l.append(segment.code);
                m_operands.append(segment.code);
            }
            l.append(segment.continuation);
            l.append(segment.ignore);
        }

        std::string_view operands = m_operands;
        bool is_include = false;
        if (consume(operands, INCLUDE_literal))
        {
            if (remove_space(operands))
            {
                // trim right spaces
                const auto end_of_operands = operands.find_last_not_of(' ');
                if (end_of_operands != std::string_view::npos)
                    operands = operands.substr(0, end_of_operands + 1);

                if (include_allowed)
                    is_include = true;
                else if (m_diags)
                    m_diags(diagnostic_op::error_P0003(range(position(lineno, 0)), operands));
            }
        }

        if (is_include)
            process_include(operands, lineno, queue);
        else
            queue.push_back("***$$$");

        return true;
    }

    // Inherited via preprocessor
    virtual bool fill_buffer(std::string_view& input, size_t lineno, std::deque<std::string>& queue) override
    {
        return fill_buffer(input, lineno, queue, true);
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
