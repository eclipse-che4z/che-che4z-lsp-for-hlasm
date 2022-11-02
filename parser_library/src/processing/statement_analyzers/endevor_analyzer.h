/*
 * Copyright (c) 2022 Broadcom.
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

#ifndef PROCESSING_ENDEVOR_ANALYZER_H
#define PROCESSING_ENDEVOR_ANALYZER_H

#include "context/hlasm_context.h"
#include "occurence_collector.h"
#include "processing/statement.h"
#include "statement_analyzer.h"
#include "analyzing_context.h"

namespace hlasm_plugin::parser_library::processing {

class endevor_analyzer : public statement_analyzer
{
public:
    endevor_analyzer(analyzing_context ctx, const std::string& text);

    void analyze(const context::hlasm_statement& statement,
        statement_provider_kind prov_kind,
        processing_kind proc_kind) override;

private:
    analyzing_context m_ctx;
    const std::string& m_text;
    lsp::occurence_storage m_stmt_occurences;
    lsp::file_occurences_t m_opencode_occurences;
    lsp::vardef_storage m_opencode_var_defs;

    void analyze_preproc(
        const semantics::endevor_statement& statement, statement_provider_kind prov_kind, processing_kind proc_kind);

    void add_copy_operand(context::id_index name, const range& operand_range);

    void collect_occurence(const semantics::instruction_si& instruction, occurence_collector& collector);
};

} // namespace hlasm_plugin::parser_library::processing

#endif