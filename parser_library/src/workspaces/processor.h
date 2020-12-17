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

#ifndef HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_H
#define HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_H

#include <memory>

#include "context/hlasm_context.h"
#include "diagnosable.h"
#include "file.h"
#include "parse_lib_provider.h"
#include "semantics/lsp_info_processor.h"

namespace hlasm_plugin::parser_library::workspaces {

// Interface that represents an object that can be parsed.
// The only implementor is processor_file
class processor : public virtual diagnosable
{
public:
    virtual bool parse_info_updated() = 0;

    // starts parser with new (empty) context
    virtual parse_result parse(parse_lib_provider&) = 0;
    // starts parser with in the context of parameter
    virtual parse_result parse_macro(parse_lib_provider&, context::hlasm_context&, const library_data) = 0;
    // starts parser to parse macro but does not update parse info or diagnostics
    virtual parse_result parse_no_lsp_update(parse_lib_provider&, context::hlasm_context&, const library_data) = 0;
};

// Interface that represents a file that can be parsed.
class processor_file : public virtual file, public processor
{
public:
    virtual const std::set<std::string>& dependencies() = 0;
    virtual const semantics::lsp_info_processor& get_lsp_info() = 0;
    virtual const std::set<std::string>& files_to_close() = 0;
    virtual const performance_metrics& get_metrics() = 0;
};

} // namespace hlasm_plugin::parser_library::workspaces
#endif