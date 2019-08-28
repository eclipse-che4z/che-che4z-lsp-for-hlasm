#ifndef SEMANTICS_COLLECTOR_H
#define SEMANTICS_COLLECTOR_H

#include "statement.h"
#include "antlr4-runtime.h"
#include "lsp_info_processor.h"

#include <variant>
#include "optional"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

//class containing methods for collecting parsed statement fields
class collector
{
public:
	collector();
	const label_si& current_label();
	const instruction_si& current_instruction();
	const operands_si& current_operands();
	const remarks_si& current_remarks();

	void set_label_field(range symbol_range);
	void set_label_field(std::string label, range symbol_range);
	void set_label_field(seq_sym sequence_symbol, range symbol_range);
	void set_label_field(context::id_index label, antlr4::ParserRuleContext* ctx, range symbol_range);
	void set_label_field(concat_chain label, range symbol_range);

	void set_instruction_field(range symbol_range);
	void set_instruction_field(context::id_index instr, range symbol_range);
	void set_instruction_field(concat_chain instr, range symbol_range);

	void set_operand_remark_field(range symbol_range);
	void set_operand_remark_field(std::string deferred, std::vector<range> remarks, range symbol_range);
	void set_operand_remark_field(std::vector<operand_ptr> operands, std::vector<range> remarks, range symbol_range);

	void add_lsp_symbol(lsp_symbol symbol);
	void add_hl_symbol(token_info symbol);

	const instruction_si& peek_instruction();
	std::variant<statement_si,statement_si_deferred> extract_statement(bool deferred_hint);
	std::vector<lsp_symbol> extract_lsp_symbols();
	std::vector<token_info> extract_hl_symbols();
	void prepare_for_next_statement();

	//range_offset range_offs;
private:
	std::optional<label_si> lbl_;
	std::optional<instruction_si> instr_;
	std::optional<operands_si> op_;
	std::optional<remarks_si> rem_;
	std::optional<std::pair<std::string,range>> def_;

	std::vector<lsp_symbol> lsp_symbols_;
	std::vector<token_info> hl_symbols_;
	bool lsp_symbols_extracted_;
	bool hl_symbols_extracted_;
};

}
}
}
#endif
