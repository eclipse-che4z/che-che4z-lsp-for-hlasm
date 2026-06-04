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

#include <memory>
#include <optional>
#include <unordered_map>

#include "checking/data_definition/data_def_type_base.h"
#include "low_language_processor.h"

namespace hlasm_plugin::parser_library {
struct analyzing_context;
class diagnosable_ctx;
struct range;
class output_handler;
class parse_lib_provider;
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::context {
enum class section_kind : std::uint8_t;
} // namespace hlasm_plugin::parser_library::context

namespace hlasm_plugin::parser_library::processing {
class branching_provider;
class opencode_provider;
class processing_manager;
class statement_fields_parser;

struct rebuit_statement;
struct resolved_statement;
} // namespace hlasm_plugin::parser_library::processing

namespace hlasm_plugin::parser_library::processing {

// processor of assembler instructions
class asm_processor final : public low_language_processor
{
    struct handler_table;

public:
    asm_processor(const analyzing_context& ctx,
        branching_provider& branch_provider,
        parse_lib_provider& lib_provider,
        statement_fields_parser& parser,
        opencode_provider& open_code,
        const processing_manager& proc_mgr,
        output_handler* output,
        diagnosable_ctx& diag_ctx);

    void process(std::shared_ptr<const processing::resolved_statement> stmt) override;
    struct extract_copy_id_result
    {
        context::id_index name;
        range operand;
        range statement;
    };
    static std::optional<extract_copy_id_result> extract_copy_id(
        const processing::resolved_statement& stmt, diagnosable_ctx* diagnoser);
    static bool common_copy_postprocess(bool processed,
        const extract_copy_id_result& data,
        context::hlasm_context& hlasm_ctx,
        diagnosable_ctx* diagnoser);

private:
    opencode_provider* open_code_;
    output_handler* output;

    context::id_index find_sequence_symbol(const rebuilt_statement& stmt);

    void process_sect(rebuilt_statement&& stmt, const context::section_kind kind);
    void process_LOCTR(rebuilt_statement&& stmt);
    void process_EQU(rebuilt_statement&& stmt);
    void process_DC(rebuilt_statement&& stmt);
    void process_DS(rebuilt_statement&& stmt);
    void process_COPY(rebuilt_statement&& stmt);
    void process_DXD(rebuilt_statement&& stmt);
    void process_EXTRN(rebuilt_statement&& stmt);
    void process_WXTRN(rebuilt_statement&& stmt);
    void process_ORG(rebuilt_statement&& stmt);
    void process_OPSYN(rebuilt_statement&& stmt);
    void process_AINSERT(rebuilt_statement&& stmt);
    void process_CCW(rebuilt_statement&& stmt);
    void process_CNOP(rebuilt_statement&& stmt);
    void process_START(rebuilt_statement&& stmt);
    void process_ALIAS(rebuilt_statement&& stmt);
    void process_END(rebuilt_statement&& stmt);
    void process_LTORG(rebuilt_statement&& stmt);
    void process_USING(rebuilt_statement&& stmt);
    void process_DROP(rebuilt_statement&& stmt);
    void process_PUSH(rebuilt_statement&& stmt);
    void process_POP(rebuilt_statement&& stmt);
    void process_MNOTE(rebuilt_statement&& stmt);
    void process_CXD(rebuilt_statement&& stmt);
    void process_TITLE(rebuilt_statement&& stmt);
    void process_PUNCH(rebuilt_statement&& stmt);
    void process_CATTR(rebuilt_statement&& stmt);
    void process_ENTRY(rebuilt_statement&& stmt);
    void process_ADATA(rebuilt_statement&& stmt);
    struct cattr_ops_result
    {
        size_t op_count;
        context::id_index part;
        range part_range;
    };
    cattr_ops_result cattr_ops(const semantics::operands_si& ops);
    void handle_cattr_ops(context::id_index class_name,
        context::id_index part_name,
        const range& part_rng,
        size_t op_count,
        const rebuilt_statement& stmt);
    void process_XATTR(rebuilt_statement&& stmt);

    template<checking::data_instr_type instr_type>
    void process_data_instruction(rebuilt_statement&& stmt);

    enum class external_type
    {
        strong,
        weak,
    };

    void process_external(rebuilt_statement&& stmt, external_type t);
};

} // namespace hlasm_plugin::parser_library::processing
#endif
