#ifndef CONTEXT_LSP_CONTEXT_H
#define CONTEXT_LSP_CONTEXT_H

#include "../semantics/highlighting_info.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {


struct completion_item_s
{
	std::string label;
	size_t kind;
	std::string detail;
	std::string documentation;
	bool deprecated;
	std::string insert_text;
};

struct definition
{
	definition() {};
	definition(std::string name, std::string scope, std::string file_name, range definition_range, std::vector<std::string> value = { "" }, bool check_scopes = true) : name(std::move(name)), scope(std::move(scope)), file_name(std::move(file_name)), definition_range(definition_range), value(value), check_scopes(check_scopes) {}

	std::string name;
	std::string scope;
	std::string file_name;
	range definition_range;
	std::vector<std::string> value;
	bool check_scopes = false;
	size_t version = 0;

	bool operator<(const definition& other) const;
	bool operator==(const definition& other) const;
	bool operator!=(const definition& other) const;
};

struct occurence
{
	range definition_range;
	std::string file_name;
};

using definitions = std::multimap<definition, occurence>;

struct lsp_context
{
	definitions seq_symbols;
	definitions var_symbols;
	definitions ord_symbols;
	definitions instructions;

	std::vector<definition> deferred_seqs;
	std::vector<definition> deferred_ord_defs;
	std::vector<definition> deferred_ord_occs;
	std::stack<std::string> parser_macro_stack;
	definition deferred_macro_statement;

	bool copy = false;

	bool initialized = false;
	std::vector<completion_item_s> all_instructions;
};

using lsp_ctx_ptr = std::shared_ptr<lsp_context>;
}
}
}
#endif