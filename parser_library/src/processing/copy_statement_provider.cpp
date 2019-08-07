#include "copy_statement_provider.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

copy_statement_provider::copy_statement_provider(context::hlasm_context& hlasm_ctx, statement_field_reparser& parser)
	:common_statement_provider(statement_provider_kind::COPY, hlasm_ctx, parser),initial_nest_(hlasm_ctx.copy_stack().size()) {}

void copy_statement_provider::process_next(statement_processor& processor)
{
	if (finished())
		throw std::runtime_error("provider already finished");

	auto& invo = hlasm_ctx.copy_stack().back();

	++invo.current_statement;
	if ((size_t)invo.current_statement == invo.definition.size())
	{
		hlasm_ctx.leave_copy_member();
		return;
	}

	const context::shared_stmt_ptr& stmt = invo.definition[invo.current_statement];

	switch (stmt->kind)
	{
	case context::statement_kind::RESOLVED:
		processor.process_statement(stmt);
		break;
	case context::statement_kind::DEFERRED:
		preprocess_deferred(processor, stmt);
		break;
	default:
		break;
	}
}

bool copy_statement_provider::finished() const
{
	return hlasm_ctx.copy_stack().size() <= initial_nest_;
}
