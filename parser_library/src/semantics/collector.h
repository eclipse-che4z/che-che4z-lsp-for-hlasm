#ifndef SEMANTICS_COLLECTOR_H
#define SEMANTICS_COLLECTOR_H

#include "statement_fields.h"
#include "semantic_objects.h"
#include "lsp_info_processor.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

//class containing methods for collecting parsed statement fields
class collector
{
public:
	collector();
	const label_semantic_info& current_label();
	const instruction_semantic_info& current_instruction();
	const operand_remark_semantic_info& current_operands_and_remarks();

	void set_label_field(symbol_range range);
	void set_label_field(std::string label, symbol_range range);
	void set_label_field(seq_sym sequence_symbol, symbol_range range);
	void set_label_field(std::string label, antlr4::ParserRuleContext* ctx);
	void set_label_field(concat_chain label, symbol_range range);

	void set_instruction_field(symbol_range range);
	void set_instruction_field(std::string instr, symbol_range range);
	void set_instruction_field(concat_chain instr, symbol_range range);

	void set_operand_remark_field(symbol_range range);
	void set_operand_remark_field(concat_chain chain, symbol_range range);
	void set_operand_remark_field(std::vector<operand_ptr> operands, std::vector<symbol_range> remarks);

	void set_statement_range(symbol_range range);

	void add_lsp_symbol(lsp_symbol symbol);
	void add_hl_symbol(token_info symbol);

	instruction_semantic_info&& extract_instruction_field();
	statement&& extract_statement();
	std::vector<lsp_symbol>&& extract_lsp_symbols();
	std::vector<token_info>&& extract_hl_symbols();
	void prepare_for_next_statement();

private:
	statement stmt_;
	std::vector<lsp_symbol> lsp_symbols_;
	std::vector<token_info> hl_symbols_;
	bool instruction_extracted_;
	bool statement_extracted_;
	bool lsp_symbols_extracted_;
	bool hl_symbols_extracted_;
};

}
}
}
#endif
