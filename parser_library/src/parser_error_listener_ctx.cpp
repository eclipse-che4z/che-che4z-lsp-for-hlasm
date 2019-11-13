#include "parser_error_listener_ctx.h"

using namespace hlasm_plugin::parser_library;

parser_error_listener_ctx::parser_error_listener_ctx
(context::hlasm_context& hlasm_ctx, std::optional<std::string> substituted, semantics::range_provider provider)
	:diagnosable_ctx(hlasm_ctx), substituted_(std::move(substituted)),provider_(std::move(provider)) {}

void hlasm_plugin::parser_library::parser_error_listener_ctx::collect_diags() const {}

void parser_error_listener_ctx::add_parser_diagnostic(range diagnostic_range, diagnostic_severity severity, std::string code, std::string message)
{
	if (substituted_)
		message = "While substituting to '" + *substituted_ + "' => " + message;
	add_diagnostic(diagnostic_op(severity, std::move(code), std::move(message), provider_.adjust_range(diagnostic_range)));
}
