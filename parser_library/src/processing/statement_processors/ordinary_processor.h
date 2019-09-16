#ifndef PROCESSING_ORDINARY_PROCESSOR_H
#define PROCESSING_ORDINARY_PROCESSOR_H

#include "statement_processor.h"
#include "../instruction_sets/ca_processor.h"
#include "../instruction_sets/macro_processor.h"
#include "../instruction_sets/asm_processor.h"
#include "../instruction_sets/mach_processor.h"
#include "../../parse_lib_provider.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//statement processor that evaluates the writen code, processes instructions
class ordinary_processor : public statement_processor
{
	static constexpr size_t NEST_LIMIT = 100;
	static constexpr size_t ACTR_LIMIT = 100;

	parse_lib_provider& lib_provider_;

	ca_processor ca_proc_;
	macro_processor mac_proc_;
	asm_processor asm_proc_;
	mach_processor mach_proc_;

	bool finished_flag_;
public:
	ordinary_processor(
		context::hlasm_context& hlasm_ctx, 
		parse_lib_provider& lib_provider, 
		branching_provider& branch_provider, 
		processing_state_listener& state_listener,
		statement_fields_parser& parser);

	virtual processing_status get_processing_status(const semantics::instruction_si& instruction) const override;
	virtual void process_statement(context::unique_stmt_ptr statement) override;
	virtual void process_statement(context::shared_stmt_ptr statement) override;
	virtual void end_processing() override;
	virtual bool terminal_condition(const statement_provider_kind kind) const override;
	virtual bool finished() override;

	static std::optional<processing_status> get_instruction_processing_status(context::id_index instruction, context::hlasm_context& hlasm_ctx);

	virtual void collect_diags() const override;
private:
	std::pair<std::vector<context::id_index>, bool> check_layout();
	void check_postponed_statements(std::vector<context::post_stmt_ptr> stmts);
	bool check_fatals(range line_range);

	template <typename T>
	void process_statement_base(T statement)
	{
		assert(statement->kind == context::statement_kind::RESOLVED);

		bool fatal = check_fatals(range(statement->statement_position()));
		if (fatal) return;

		switch (statement->access_resolved()->opcode_ref().type)
		{
		case context::instruction_type::UNDEF:
			add_diagnostic(diagnostic_op::error_E049(*statement->access_resolved()->opcode_ref().value, statement->access_resolved()->instruction_ref().field_range));
			return;
		case context::instruction_type::CA:
			ca_proc_.process(std::move(statement));
			return;
		case context::instruction_type::MAC:
			mac_proc_.process(std::move(statement));
			return;
		case context::instruction_type::ASM:
			asm_proc_.process(std::move(statement));
			return;
		case context::instruction_type::MACH:
			mach_proc_.process(std::move(statement));
			return;
		default:
			assert(false);
			return;
		}
	}
};

}
}
}
#endif
