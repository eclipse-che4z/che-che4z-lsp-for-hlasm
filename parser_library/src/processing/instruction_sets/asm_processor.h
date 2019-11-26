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

#ifndef PROCESSING_ASM_PROCESSOR_H
#define PROCESSING_ASM_PROCESSOR_H

#include "low_language_processor.h"
#include "../../parse_lib_provider.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//processor of assembler instructions
class asm_processor : public low_language_processor
{
	using process_table_t = std::unordered_map<context::id_index, std::function<void(rebuilt_statement)>>;

	const process_table_t table_;
	checking::assembler_checker checker_;
public:
	asm_processor(context::hlasm_context& hlasm_ctx, 
		attribute_provider& attr_provider, branching_provider& branch_provider, parse_lib_provider& lib_provider,
		statement_fields_parser& parser);

	virtual void process(context::unique_stmt_ptr stmt) override;
	virtual void process(context::shared_stmt_ptr stmt) override;

	static void process_copy(const semantics::complete_statement& stmt, context::hlasm_context& hlasm_ctx, parse_lib_provider& lib_provider, diagnosable_ctx* diagnoser);

private:
	process_table_t create_table(context::hlasm_context& hlasm_ctx);

	context::id_index find_sequence_symbol(const rebuilt_statement& stmt);

	void process(rebuilt_statement statement);

	void process_sect(const context::section_kind kind, rebuilt_statement stmt);
	void process_LOCTR(rebuilt_statement stmt);
	void process_EQU(rebuilt_statement stmt);
	void process_DC(rebuilt_statement stmt);
	void process_DS(rebuilt_statement stmt);
	void process_COPY(rebuilt_statement stmt);

	template<checking::data_instr_type instr_type>
	void process_data_instruction(rebuilt_statement stmt);
};

}
}
}
#endif