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
#include "processing/statement_processors/copy_processing_info.h"
#include "processing/statement_processors/macrodef_processing_info.h"
#include "processing/statement_providers/statement_provider_kind.h"
#include "processing_format.h"

namespace hlasm_plugin::parser_library::processing {

class statement_analyzer;

using analyzer_ptr = std::unique_ptr<statement_analyzer>;

class statement_analyzer
{
public:
    virtual bool analyze(const context::hlasm_statement& statement,
        statement_provider_kind prov_kind,
        processing_kind proc_kind,
        bool evaluated_model) = 0;

    virtual void analyze_aread_line(
        const utils::resource::resource_location& rl, size_t lineno, std::string_view text) = 0;

protected:
    ~statement_analyzer() = default;
};

} // namespace hlasm_plugin::parser_library::processing

#endif
