#include "macro_statement_provider.h"
#include "processing_manager.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

macro_statement_provider::macro_statement_provider(context::hlasm_context& hlasm_ctx, statement_field_reparser& parser)
	:statement_provider(statement_provider_kind::MACRO), hlasm_ctx_(hlasm_ctx), parser_(parser) {}

void macro_statement_provider::process_next(statement_processor& processor)
{
	if (!hlasm_ctx_.is_in_macro())
		throw std::runtime_error("provider already finished");

	auto& invo = hlasm_ctx_.scope_stack().back().this_macro;
	assert(invo);

	++invo->current_statement;
	if ((size_t)invo->current_statement == invo->definition.size())
	{
		hlasm_ctx_.leave_macro();
		return;
	}

	const context::shared_stmt_ptr& stmt = invo->definition[invo->current_statement];

	switch (stmt->kind)
	{
	case context::statement_kind::RESOLVED:
		processor.process_statement(stmt);
		break;
	case context::statement_kind::DEFERRED:
		preprocess(processor, stmt);
		break;
	default:
		break;
	}
}

bool macro_statement_provider::finished() const
{
	return !hlasm_ctx_.is_in_macro();
}
void macro_statement_provider::preprocess(statement_processor& processor, context::shared_stmt_ptr stmt)
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

		if (status.first.occurence == operand_occurence::ABSENT)
		{
			semantics::operands_si op(def_stmt.deferred_range_ref(), semantics::operand_list());
			semantics::remarks_si rem(def_stmt.deferred_range_ref(), {});

			ptr = std::make_unique<resolved_statement_impl>(semantics::statement_si_defer_done(*def_impl, std::move(op), std::move(rem)), status.second);
		}
		else
		{
			auto [op, rem] = parser_.reparse_operand_field(
				&hlasm_ctx_,
				def_stmt.deferred_ref(),
				semantics::range_provider(def_stmt.deferred_range_ref(),false),
				status);

			ptr = std::make_unique<resolved_statement_impl>(semantics::statement_si_defer_done(*def_impl, std::move(op), std::move(rem)), status.second);
		}

		processor.process_statement(std::move(ptr));
	}
}
