#ifndef CONTEXT_HLASM_STATEMENT_H
#define CONTEXT_HLASM_STATEMENT_H

#include <memory>
#include <vector>
#include "../include/shared/range.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {
struct resolved_statement;
}
namespace semantics {
struct deferred_statement;
}
namespace context {

struct hlasm_statement;

using shared_stmt_ptr = std::shared_ptr<const hlasm_statement>;
using unique_stmt_ptr = std::unique_ptr<hlasm_statement>;

using statement_block = std::vector<shared_stmt_ptr>;

enum class statement_kind
{
	RESOLVED, DEFERRED
};

//abstract structure representing general hlasm statement
struct hlasm_statement
{
	const statement_kind kind;

	const processing::resolved_statement* access_resolved() const;
	processing::resolved_statement* access_resolved();

	const semantics::deferred_statement* access_deferred() const;
	semantics::deferred_statement* access_deferred();

	virtual position statement_position() const = 0;

	virtual ~hlasm_statement() = default;

protected:
	hlasm_statement(const statement_kind kind);
};


}
}
}

#endif