#include "diagnostic_collector.h"
#include "diagnosable_ctx.h"

namespace hlasm_plugin::parser_library {

diagnostic_collector::diagnostic_collector(diagnosable_ctx* diagnoser, context::processing_stack_t location_stack)
	: diagnoser_(diagnoser), location_stack_(std::move(location_stack)) {}

diagnostic_collector::diagnostic_collector(diagnosable_ctx* diagnoser)
	: diagnoser_(diagnoser), location_stack_(diagnoser->ctx_.processing_stack()) {}

diagnostic_collector::diagnostic_collector()
	: diagnoser_(nullptr) {}

void diagnostic_collector::operator()(diagnostic_op diagnostic) const
{
	if (!diagnoser_) return;
	diagnoser_->add_diagnostic_inner(std::move(diagnostic), location_stack_);
}

}