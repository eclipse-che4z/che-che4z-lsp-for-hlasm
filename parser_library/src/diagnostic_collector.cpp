#include "diagnostic_collector.h"
#include "diagnosable_ctx.h"

namespace hlasm_plugin::parser_library {

diagnostic_collector::diagnostic_collector(diagnosable_ctx* diagnoser, std::vector<location> location_stack)
	: diagnoser_(diagnoser), location_stack_(std::move(location_stack)) {}

diagnostic_collector::diagnostic_collector(diagnosable_ctx* diagnoser)
	: diagnoser_(diagnoser), location_stack_(diagnoser->ctx_.processing_stack()) {}

diagnostic_collector::diagnostic_collector()
	: diagnoser_(nullptr) {}

void diagnostic_collector::operator()(diagnostic_op diagnostic) const
{
	if (!diagnoser_) return;
	diagnostic_s tmp_diag(diagnostic.range_,std::move(diagnostic));
	diagnoser_->add_diagnostic_inner(std::move(tmp_diag), location_stack_);
}

}