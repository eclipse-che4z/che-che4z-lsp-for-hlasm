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

	parse_lib_provider& lib_provider_;
public:
	asm_processor(context::hlasm_context& hlasm_ctx, branching_provider& branch_provider, parse_lib_provider& lib_provider, statement_field_reparser& parser);

	virtual void process(context::unique_stmt_ptr stmt) override;
	virtual void process(context::shared_stmt_ptr stmt) override;

	static void process_copy(const semantics::complete_statement& stmt, context::hlasm_context& hlasm_ctx, parse_lib_provider& lib_provider, diagnosable* diagnoser);

private:
	process_table_t create_table(context::hlasm_context& hlasm_ctx);

	context::id_index find_label_symbol(const rebuilt_statement& stmt);

	context::id_index find_sequence_symbol(const rebuilt_statement& stmt);

	void process(rebuilt_statement statement);

	void process_sect(context::section_kind kind,const rebuilt_statement stmt);
	void process_LOCTR(rebuilt_statement stmt);
	void process_EQU(rebuilt_statement stmt);
	void process_DC(rebuilt_statement stmt);
	void process_DS(rebuilt_statement stmt);
	void process_COPY(rebuilt_statement stmt);
};

}
}
}
#endif