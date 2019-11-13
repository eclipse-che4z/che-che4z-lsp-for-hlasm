#include "common_statement_provider.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

common_statement_provider::common_statement_provider(const statement_provider_kind kind, context::hlasm_context& hlasm_ctx, statement_fields_parser& parser)
	: statement_provider(kind), hlasm_ctx(hlasm_ctx),parser(parser) {}

void common_statement_provider::preprocess_deferred(statement_processor& processor, context::shared_stmt_ptr stmt)
{
	const auto& def_stmt = *stmt->access_deferred();

	auto status = processor.get_processing_status(def_stmt.instruction_ref());

	if (status.first.form == processing_form::DEFERRED)
	{
		processor.process_statement(stmt);
	}
	else
	{
		context::unique_stmt_ptr ptr;
		auto def_impl = dynamic_cast<const semantics::statement_si_deferred*>(&def_stmt);

		if (status.first.occurence == operand_occurence::ABSENT || 
			status.first.form == processing_form::UNKNOWN || status.first.form == processing_form::IGNORED)
		{
			semantics::operands_si op(def_stmt.deferred_range_ref(), semantics::operand_list());
			semantics::remarks_si rem(def_stmt.deferred_range_ref(), {});

			ptr = std::make_unique<resolved_statement_impl>(semantics::statement_si_defer_done(*def_impl, std::move(op), std::move(rem)), status.second);
		}
		else
		{
			auto [op, rem] = parser.parse_operand_field(
				&hlasm_ctx,
				def_stmt.deferred_ref(),
				false,
				semantics::range_provider(def_stmt.deferred_range_ref(), semantics::adjusting_state::NONE),
				status);

			ptr = std::make_unique<resolved_statement_impl>(semantics::statement_si_defer_done(*def_impl, std::move(op), std::move(rem)), status.second);
		}

		processor.process_statement(std::move(ptr));
	}
}