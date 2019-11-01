#include "parser_error_listener_ctx.h"

using namespace hlasm_plugin::parser_library;

parser_error_listener_ctx::parser_error_listener_ctx(context::hlasm_context& hlasm_ctx, std::optional<std::string> substituted)
	:diagnosable_ctx(hlasm_ctx), substituted_(std::move(substituted)) {}

void hlasm_plugin::parser_library::parser_error_listener_ctx::collect_diags() const {}

void parser_error_listener_ctx::add_parser_diagnostic(range diagnostic_range, diagnostic_severity severity, std::string code, std::string message)
{
	if (substituted_)
		message = "While substituting to '" + *substituted_ + "' => " + message;
	add_diagnostic(diagnostic_op(severity, std::move(code), std::move(message), diagnostic_range));
}
