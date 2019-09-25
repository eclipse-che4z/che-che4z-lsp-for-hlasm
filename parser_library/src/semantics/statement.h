#ifndef SEMANTICS_STATEMENT_H
#define SEMANTICS_STATEMENT_H

#include "statement_fields.h"
#include "../context/hlasm_statement.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

struct core_statement
{
	virtual const range& stmt_range_ref() const = 0;
	virtual const label_si& label_ref() const = 0;
	virtual const instruction_si& instruction_ref() const = 0;

	virtual ~core_statement() = default;
};

struct complete_statement :public core_statement
{
	virtual const operands_si& operands_ref() const = 0;
	virtual const remarks_si& remarks_ref() const = 0;
};

struct deferred_statement : public context::hlasm_statement, public core_statement
{
	virtual const std::string& deferred_ref() const = 0;
	virtual const range& deferred_range_ref() const = 0;

	virtual position statement_position() const override { return stmt_range_ref().start; }

	deferred_statement()
		: hlasm_statement(context::statement_kind::DEFERRED) {}
};

struct statement_si_deferred : public deferred_statement
{
	statement_si_deferred(
		range stmt_range,
		label_si label,
		instruction_si instruction,
		std::string deferred_field,
		range deferred_range)
		: stmt_range(std::move(stmt_range)),
		label(std::move(label)),
		instruction(std::move(instruction)),
		deferred_field(std::move(deferred_field)),
		deferred_range(deferred_range) {}

	range stmt_range;

	label_si label;
	instruction_si instruction;
	std::string deferred_field;
	range deferred_range;

	virtual const label_si& label_ref() const { return label; };
	virtual const instruction_si& instruction_ref() const { return instruction; };
	virtual const std::string& deferred_ref() const { return deferred_field; };
	virtual const range& deferred_range_ref() const { return deferred_range; };
	virtual const range& stmt_range_ref() const { return stmt_range; };
};

//struct holding full semantic information (si) about whole instruction statement, whole logical line
struct statement_si : public complete_statement
{
	statement_si(
		range stmt_range,
		label_si label,
		instruction_si instruction,
		operands_si operands,
		remarks_si remarks)
		: stmt_range(std::move(stmt_range)),
		label(std::move(label)),
		instruction(std::move(instruction)),
		operands(std::move(operands)),
		remarks(std::move(remarks)) {}

	range stmt_range;

	label_si label;
	instruction_si instruction;
	operands_si operands;
	remarks_si remarks;

	virtual const label_si& label_ref() const { return label; }
	virtual const instruction_si& instruction_ref() const { return instruction; }
	virtual const operands_si& operands_ref() const { return operands; }
	virtual const remarks_si& remarks_ref() const { return remarks; }
	virtual const range& stmt_range_ref() const { return stmt_range; }
};

struct statement_si_defer_done : public complete_statement
{
	statement_si_defer_done(
		const statement_si_deferred& deferred_stmt,
		operands_si operands,
		remarks_si remarks)
		: deferred_stmt(&deferred_stmt),
		operands(std::move(operands)),
		remarks(std::move(remarks)) {}

	const statement_si_deferred* deferred_stmt;

	operands_si operands;
	remarks_si remarks;

	virtual const label_si& label_ref() const { return deferred_stmt->label; }
	virtual const instruction_si& instruction_ref() const { return deferred_stmt->instruction; }
	virtual const operands_si& operands_ref() const { return operands; }
	virtual const remarks_si& remarks_ref() const { return remarks; }
	virtual const range& stmt_range_ref() const { return deferred_stmt->stmt_range; }
};

}
}
}
#endif
