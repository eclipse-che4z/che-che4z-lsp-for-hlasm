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

#ifndef PROCESSING_LOW_LANGUAGE_PROCESSOR_H
#define PROCESSING_LOW_LANGUAGE_PROCESSOR_H

#include "checking/instruction_checker.h"
#include "instruction_processor.h"
#include "processing/statement_fields_parser.h"

namespace hlasm_plugin::parser_library::processing {
class processing_manager;

// common ancestor for ASM and MACH processing containing useful methods
class low_language_processor : public instruction_processor
{
protected:
    statement_fields_parser& parser;
    const processing_manager& proc_mgr;

    low_language_processor(analyzing_context ctx,
        branching_provider& branch_provider,
        workspaces::parse_lib_provider& lib_provider,
        statement_fields_parser& parser,
        const processing_manager& proc_mgr);

    rebuilt_statement preprocess(std::shared_ptr<const processing::resolved_statement> stmt);

    // finds symbol in the label field
    context::id_index find_label_symbol(const rebuilt_statement& stmt) const;
    // finds using label in the label field
    context::id_index find_using_label(const rebuilt_statement& stmt) const;

    // helper method to create symbol
    bool create_symbol(range err_range,
        context::id_index symbol_name,
        context::symbol_value value,
        context::symbol_attributes attributes);


private:
    struct preprocessed_part
    {
        std::optional<semantics::label_si> label;
        std::optional<semantics::operands_si> operands;
        std::optional<std::vector<semantics::literal_si>> literals;
        bool was_model = false;
    };
    preprocessed_part preprocess_inner(const resolved_statement& stmt);
};

enum class check_org_result
{
    valid,
    underflow,
    invalid_address,
};

// helper method to check address for the ORG instruction
check_org_result check_address_for_ORG(
    const context::address& addr_to_check, const context::address& curr_addr, size_t boundary, int offset);

} // namespace hlasm_plugin::parser_library::processing
#endif
