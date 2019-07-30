#include "empty_processor.h"
#include <utility>

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

empty_processor::empty_processor(context::hlasm_context& hlasm_ctx)
	: statement_processor(processing_kind::ORDINARY, hlasm_ctx) {}

processing_status empty_processor::get_processing_status(const semantics::instruction_si&) const
{
	return std::make_pair(processing_format(processing_kind::ORDINARY, processing_form::UNKNOWN), op_code());
}

void empty_processor::process_statement(context::unique_stmt_ptr) {}

void empty_processor::process_statement(context::shared_stmt_ptr) {}

void empty_processor::end_processing() {}

bool empty_processor::terminal_condition(const statement_provider_kind) const { return true; }

bool empty_processor::finished() { return true; }

void empty_processor::collect_diags() const {}
