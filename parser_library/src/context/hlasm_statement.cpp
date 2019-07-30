#include "hlasm_statement.h"
#include "../processing/statement.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;

const processing::resolved_statement* hlasm_statement::access_resolved() const 
{ 
	return kind == statement_kind::RESOLVED ? static_cast<const processing::resolved_statement*>(this) : nullptr; 
}
processing::resolved_statement* hlasm_statement::access_resolved() 
{
	return kind == statement_kind::RESOLVED ? static_cast<processing::resolved_statement*>(this) : nullptr;
}

const semantics::deferred_statement* hlasm_statement::access_deferred() const 
{
	return kind == statement_kind::DEFERRED ? static_cast<const semantics::deferred_statement*>(this) : nullptr;
}
semantics::deferred_statement* hlasm_statement::access_deferred() 
{
	return kind == statement_kind::DEFERRED ? static_cast<semantics::deferred_statement*>(this) : nullptr;
}

hlasm_statement::hlasm_statement(const statement_kind kind)
	: kind(kind) {}
