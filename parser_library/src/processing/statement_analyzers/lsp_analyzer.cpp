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

#include "lsp_analyzer.h"

namespace hlasm_plugin::parser_library::processing {

void lsp_analyzer::analyze(
    const context::hlasm_statement& statement, statement_provider_kind prov_kind, processing_kind proc_kind)
{
    switch (proc_kind)
    {
        case processing_kind::ORDINARY:
            collect_ord_occurences(statement);
            if (prov_kind != statement_provider_kind::MACRO) // macros already processed during macro def processing
            {
                collect_var_occurences(statement);
                collect_var_definitions(statement);
                collect_seq_occurences(statement);
            }
            collect_var_seq_scope();
            break;
        case processing_kind::MACRO:
            collect_var_occurences(statement);
            collect_var_definitions(statement);
            collect_seq_occurences(statement);
            collect_var_seq_scope();
            break;
        default:
            break;
    }
}

} // namespace hlasm_plugin::parser_library::processing
