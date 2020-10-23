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

#ifndef PROCESSING_STATEMENT_ANALYZER_H
#define PROCESSING_STATEMENT_ANALYZER_H

#include "context/hlasm_statement.h"
#include "processing/processing_format.h"
#include "processing/statement_processors/macrodef_processing_info.h"
#include "processing/statement_providers/statement_provider_kind.h"

namespace hlasm_plugin::parser_library::processing {

class statement_analyzer
{
public:
    virtual void analyze(
        const context::hlasm_statement& statement, statement_provider_kind prov_kind, processing_kind proc_kind) = 0;

    virtual void macrodef_started(const macrodef_start_data& data) = 0;
    virtual void macrodef_finished(context::macro_def_ptr macrodef, macrodef_processing_result& result) = 0;

    virtual ~statement_analyzer() = default;
};

} // namespace hlasm_plugin::parser_library::processing

#endif
