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

#ifndef PROCESSING_ENDEVOR_STATEMENT_PROVIDER_H
#define PROCESSING_ENDEVOR_STATEMENT_PROVIDER_H

#include "processing/opencode_provider.h"
#include "semantics/collector.h"
#include "statement_provider.h"

namespace hlasm_plugin::parser_library::processing {

class endevor_statement_provider : public statement_provider
{
public:
    //endevor_statement_provider(std::string text, analyzing_context ctx);

    endevor_statement_provider(
        std::string text, analyzing_context ctx, size_t line_no, semantics::source_info_processor& src_proc);

    context::shared_stmt_ptr get_next(const statement_processor& processor) override;
    bool finished() const override;

private:
    std::string m_text;
    analyzing_context m_ctx;
    semantics::collector m_collector;
    size_t m_line_no;
    semantics::source_info_processor& m_src_proc;
};

} // namespace hlasm_plugin::parser_library::processing

#endif PROCESSING_ENDEVOR_STATEMENT_PROVIDER_H