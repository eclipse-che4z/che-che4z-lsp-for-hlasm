#ifndef PROCESSING_POSTPONED_STATEMENT_IMPL_H
#define PROCESSING_POSTPONED_STATEMENT_IMPL_H

#include "../statement.h"
#include "../../context/ordinary_assembly/postponed_statement.h"

#include <variant>

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//implementation of postponed_statement interface
struct postponed_statement_impl : public context::postponed_statement
{
	postponed_statement_impl(rebuilt_statement stmt,std::vector<location> stmt_location_stack)
		: stmt(std::move(stmt)), stmt_location_stack(std::move(stmt_location_stack)) {}

	rebuilt_statement stmt;
	std::vector<location> stmt_location_stack;

	virtual const semantics::label_si& label_ref() const { return stmt.label_ref(); }
	virtual const semantics::instruction_si& instruction_ref() const { return stmt.instruction_ref(); }
	virtual const semantics::operands_si& operands_ref() const { return stmt.operands_ref(); }
	virtual const semantics::remarks_si& remarks_ref() const { return stmt.remarks_ref(); }
	virtual const range& stmt_range_ref() const { return stmt.stmt_range_ref(); }
	virtual const op_code& opcode_ref() const { return stmt.opcode_ref(); }

	virtual const std::vector<location>& location_stack() const { return stmt_location_stack; }
};



}
}
}
#endif