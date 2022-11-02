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


#ifndef HLASMPARSER_PARSERLIBRARY_PREPROCESSOR_ANALYZER_H
#define HLASMPARSER_PARSERLIBRARY_PREPROCESSOR_ANALYZER_H

#include <memory.h>

#include "analyzer_if.h"
#include "processing/processing_format.h"
#include "processing/statement_processors/endevor_processor.h"
#include "processing/statement_providers/statement_provider.h"
#include "processing/statement_analyzers/endevor_analyzer.h"

namespace hlasm_plugin::parser_library {
class preprocessor_analyzer : public analyzer_if
{
public:
    preprocessor_analyzer(const std::string& text,
        size_t line_no,
        analyzing_context ctx,
        processing::processing_kind proc_kind,
        workspaces::parse_lib_provider& lib_provider);

    const semantics::source_info_processor& source_processor() const override;

    void analyze(std::atomic<bool>* cancel = nullptr);

    const performance_metrics& get_metrics() const override;

    void collect_diags() const override;

private:
    size_t m_line_no;
    processing::processing_kind m_proc_kind;
    semantics::source_info_processor m_src_proc;
    std::unordered_map<processing::processing_kind, std::unique_ptr<processing::statement_provider>> m_stmt_providers;
    std::unique_ptr<processing::statement_processor> m_endevor_processor;
    std::unique_ptr<processing::endevor_analyzer> m_endevor_analyzer;
};

} // namespace hlasm_plugin::parser_library

#endif