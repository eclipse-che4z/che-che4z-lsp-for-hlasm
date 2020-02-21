/*
 * Copyright (c) 2019 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Broadcom, Inc. - initial API and implementation
 */

#ifndef CONTEXT_LSP_CONTEXT_H
#define CONTEXT_LSP_CONTEXT_H

#include "../semantics/highlighting_info.h"
#include <stack>

namespace hlasm_plugin {
namespace parser_library {
namespace context {

enum class symbol_type { seq, var, ord, instruction, hl };

constexpr size_t PRIME1 = 6291469;
constexpr size_t PRIME2 = 12582917;

struct macro_id
{
	const std::string* name;
	size_t version;

	bool operator==(const macro_id& other) const;
};

struct occurence
{
	range definition_range;
	const std::string* file_name;

	inline occurence(range range, const std::string* str)
		: definition_range(range)
		, file_name(str)
	{
	}
	inline occurence(const std::string* empty_string)
		: file_name(empty_string)
	{
	}
};

struct lsp_symbol
{
	inline lsp_symbol() : lsp_symbol(nullptr, { range(),nullptr },symbol_type::hl) {}
	inline lsp_symbol(const std::string * name, file_range symbol_range, symbol_type type)
		: name(name)
		, symbol_range(std::move(symbol_range))
		, type(type)
		, scope({ nullptr,0 }) {}

	const std::string * name;
	file_range symbol_range;
	symbol_type type;
	macro_id scope;
};

struct doc_pos
{
	doc_pos(size_t line, std::vector<std::string>* text) : line(line), text(text),defined(false) {};
	doc_pos() : line(0),text(nullptr), defined(true) {};
	size_t line;
	std::vector<std::string>* text;
	bool defined;
};

class completion_item_s
{
	std::vector<std::string> documentation;
	doc_pos doc_meta;
public:
	completion_item_s(std::string label, std::string detail, std::string insert_text, doc_pos documentation);
	completion_item_s(std::string label, std::string detail, std::string insert_text, std::vector<std::string> documentation);

	std::vector<std::string> get_documentation() const;
	void implode_documentation();
	std::string label;
	std::string detail;
	std::string insert_text;
	bool deprecated = false;
	size_t kind = 1;
	std::string documentation_string;
};

struct definition
{
	inline definition(const lsp_symbol& sym)
		: definition(sym.name, sym.symbol_range.file, sym.symbol_range.r) {}

	inline definition(const std::string* empty_string)
		: name(empty_string)
		, file_name(empty_string) {}

	inline definition(const std::string* name, const std::string* file_name, range definition_range)
		: name(name)
		, file_name(file_name)
		, definition_range(definition_range) {}


	const std::string * name;
	const std::string * file_name;
	range definition_range;

	size_t hash() const;
	std::vector<std::string> get_value() const;
	bool operator==(const definition& other) const;
};

struct ord_definition : public definition
{
	inline ord_definition() :
		definition(nullptr) {};

	inline ord_definition(const std::string* name, const std::string* file_name, range definition_range) :
		definition(name, file_name, definition_range) {}

	inline ord_definition(const lsp_symbol& sym) :
		definition(sym) {}

	inline ord_definition(const std::string* name, const std::string* file_name,
		const range& definition_range, const symbol_value val, const symbol_attributes attr) :
		definition(name, file_name, definition_range),
		val(val),
		attr(attr)
	{
	}

	const symbol_value val;
	std::optional<const symbol_attributes> attr;

	std::vector<std::string> get_value() const;
	size_t hash() const;
	bool operator==(const ord_definition& other) const;
};

enum class var_type {
	NUM, STRING, BOOL, MACRO
};

struct var_definition : public definition
{
	inline var_definition() :
		definition(nullptr), type(var_type::BOOL), scope{ nullptr, 0 } {};

	inline var_definition(const lsp_symbol& sym) :
		definition(sym), type(var_type::BOOL), scope{ nullptr, 0 } {}

	inline var_definition(const std::string* name, const std::string* file_name,
		const range& definition_range, var_type type, 
		macro_id scope) :
		definition(name, file_name, definition_range),
		type(type),
		scope(scope) {}

	var_type type;
	macro_id scope;

	std::vector<std::string> get_value() const;
	size_t hash() const;
	bool operator==(const var_definition& other) const;
};

struct seq_definition : public definition
{
	inline seq_definition() :
		definition(nullptr), scope{ nullptr, 0 } {};

	inline seq_definition(const lsp_symbol& sym) :
		definition(sym), scope{nullptr, 0} {}

	inline seq_definition(const std::string* name, const std::string* file_name,
		const range& definition_range,
		macro_id scope) :
		definition(name, file_name, definition_range),
		scope(scope) {}

	macro_id scope;
	size_t hash() const;
	std::vector<std::string> get_value() const;
	bool operator==(const seq_definition& other) const;
};

struct instr_definition : public definition
{
	inline instr_definition() :
		definition(nullptr),version(0) {};

	inline instr_definition(const lsp_symbol& sym) :
		definition(sym), version(0) {}

	inline instr_definition(const std::string* name, const std::string* file_name,
		const range definition_range, const completion_item_s& item, size_t version) :
		definition(name, file_name, definition_range),
		item(item),
		version(version) {}

	std::optional<completion_item_s> item;
	size_t version;

	std::vector<std::string> get_value() const;
	size_t hash() const;
	bool operator==(const instr_definition& other) const;
	void init(const std::string* file, const std::string* n, const range& r);
	void clear(const std::string* empty_string);
};

template<typename T>
struct hash_function
{
	size_t operator()(const T& symbol) const
	{
		return symbol.hash();
	}
};

template<class T>
using definitions = std::unordered_map<T, std::vector<occurence>, hash_function<T>>;

struct lsp_context
{
	definitions<seq_definition> seq_symbols;
	definitions<var_definition> var_symbols;
	definitions<ord_definition> ord_symbols;
	definitions<instr_definition> instructions;

	std::vector<seq_definition> deferred_seqs;
	std::vector<ord_definition> deferred_ord_defs;
	std::vector<std::pair<ord_definition,bool>> deferred_ord_occs;
	std::stack<macro_id> parser_macro_stack;
	instr_definition deferred_macro_statement;

	bool copy = false;

	bool initialized = false;
	std::vector<completion_item_s> all_instructions;

	inline lsp_context()
		: deferred_macro_statement()
		
	{
	}
};

using lsp_ctx_ptr = std::shared_ptr<lsp_context>;
}
}
}
#endif