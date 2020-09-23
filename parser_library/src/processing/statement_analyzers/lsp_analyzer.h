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

#ifndef PROCESSING_LSP_ANALYZER_H
#define PROCESSING_LSP_ANALYZER_H

#include "processing/processing_format.h"
#include "statement_analyzer.h"

namespace hlasm_plugin::parser_library::processing {

class lsp_analyzer : public statement_analyzer
{
public:
    virtual void analyze(const context::hlasm_statement& statement,
        statement_provider_kind prov_kind,
        processing_kind proc_kind) override;

private:
    void collect_ord_occurences(const context::hlasm_statement& statement);
    void collect_var_occurences(const context::hlasm_statement& statement);
    void collect_var_definitions(const context::hlasm_statement& statement);
    void collect_seq_occurences(const context::hlasm_statement& statement);
    void collect_var_seq_scope();
};

} // namespace hlasm_plugin::parser_library::processing

#endif
