#ifndef HLASMPLUGIN_PARSERLIBRARY_PARSER_IMPL_H
#define HLASMPLUGIN_PARSERLIBRARY_PARSER_IMPL_H

#include "antlr4-runtime.h"
#include "context/hlasm_context.h"
#include "semantics/collector.h"
#include "semantics/lsp_info_processor.h"
#include "parsing_format.h"
#include "common_structures.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics
{
class processing_manager;
}

//class providing methods helpful for parsing and methods modifying parsing process
class parser_impl : public antlr4::Parser
{
public:
	parser_impl(antlr4::TokenStream* input);

	void initialize(semantics::processing_manager* proc_mngr, semantics::lsp_info_processor * lsp_prc);

	bool is_last_line();
	void rewind_input(hlasm_plugin::parser_library::location location);
	semantics::operand_remark_semantic_info reparse_operand_remark_field(std::string field);
	parsing_format format;
protected:
	void enable_continuation();
	void disable_continuation();

	bool is_self_def();
	//calls processing manager to hint the parser how to parse operands
	void process_instruction();
	//calls processing manager to hint the parser how to parse following statements
	void process_statement();

	//ret={offset,line}
	hlasm_plugin::parser_library::location statement_start() const;

	semantics::processing_manager* mngr;
	semantics::lsp_info_processor* lsp_proc;
	semantics::collector collector;

	//is there just because highlighting
	//std::string file_name;
};


}
}

#endif // !HLASMPLUGIN_PARSERLIBRARY_PARSER_IMPL_H
