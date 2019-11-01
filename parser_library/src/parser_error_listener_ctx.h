#ifndef HLASMPLUGIN_PARSERLIBRARY_ERROR_LISTENER_CTX_H
#define HLASMPLUGIN_PARSERLIBRARY_ERROR_LISTENER_CTX_H

#include "parser_error_listener.h"
#include "context/hlasm_context.h"

namespace hlasm_plugin::parser_library {

class parser_error_listener_ctx : public parser_error_listener_base, public diagnosable_ctx
{
public:
	parser_error_listener_ctx(context::hlasm_context& hlasm_ctx, std::optional<std::string> substituted);

	virtual void collect_diags() const override;

protected:
	virtual void add_parser_diagnostic(range diagnostic_range, diagnostic_severity severity, std::string code, std::string message) override;

private:
	std::optional<std::string> substituted_;
};

}

#endif
