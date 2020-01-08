#include "cached_statement.h"
#include "../semantics/statement.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;

cached_statement_storage::cached_statement_storage(shared_stmt_ptr base)
	:base_stmt_(std::move(base)) {}

bool cached_statement_storage::contains(processing::processing_form format) const
{
	for (const auto& entry : cache_)
		if (entry.first == format)
			return true;
	return false;
}

void cached_statement_storage::insert(processing::processing_form format, cache_entry_t statement)
{
	cache_.emplace_back(format, std::move(statement));
}

cached_statement_storage::cache_entry_t cached_statement_storage::get(processing::processing_form format) const
{
	for (const auto& entry : cache_)
		if (entry.first == format)
			return entry.second;
	return nullptr;
}

shared_stmt_ptr cached_statement_storage::get_base() const
{
	return base_stmt_;
}
