#ifndef CONTEXT_PROCESSING_CACHED_STATEMENT_H
#define CONTEXT_PROCESSING_CACHED_STATEMENT_H

#include  "../processing/processing_format.h"
#include "hlasm_statement.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {
struct statement_si_defer_done;
}
namespace context {

class cached_statement_storage
{
public:
	using cache_entry_t = std::shared_ptr<semantics::statement_si_defer_done>;
	using cached_statement_t = std::pair<processing::processing_form, cache_entry_t>;

private:
	std::vector<cached_statement_t> cache_;
	shared_stmt_ptr base_stmt_;

public:
	cached_statement_storage(shared_stmt_ptr base);

	bool contains(processing::processing_form format) const;

	void insert(processing::processing_form format, cache_entry_t statement);

	cache_entry_t get(processing::processing_form format) const;

	shared_stmt_ptr get_base() const;
};

using cached_block = std::vector<cached_statement_storage>;

}
}
}
#endif
