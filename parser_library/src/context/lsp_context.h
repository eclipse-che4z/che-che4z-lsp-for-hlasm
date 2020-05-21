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

#include <optional>
#include <stack>
#include <unordered_map>

#include "context/ordinary_assembly/symbol.h"
#include "semantics/highlighting_info.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

// type of symbols that come from the parser
// sequence symbol, variable symbol, ordinary symbol, instruction symbol and highlighting symbol
enum class symbol_type
{
    seq,
    var,
    ord,
    instruction,
    hl
};

// prime numbers used for custom hashes
constexpr size_t PRIME1 = 6291469;
constexpr size_t PRIME2 = 12582917;

// data needed to identify a macro
struct macro_id
{
    // name of macro
    const std::string* name;
    // version of macro
    size_t version;

    bool operator==(const macro_id& other) const;
};

// represtentation of an occurence of symbol in file
struct occurence
{
    // range of the symbol in text
    range symbol_range;
    // name of file where the symbol was used
    const std::string* file_name;

    inline occurence(range range, const std::string* str)
        : symbol_range(range)
        , file_name(str)
    {}
};

// representation of an lsp symbol as it comes from parser
// later it is sorted accordingly to its type and a correct symbol is created
struct lsp_symbol
{
    inline lsp_symbol(const std::string* name, file_range symbol_range, symbol_type type)
        : name(name)
        , symbol_range(std::move(symbol_range))
        , type(type)
        , scope({ nullptr, 0 })
    {}

    // name of symbol
    const std::string* name;
    // range of the symbol along with the file name
    file_range symbol_range;
    // type of symbol
    symbol_type type;
    // macro id within which the symbol is used
    macro_id scope;
};

// help structure for completion item
// states whether and where the information about the completion item is stored
struct content_pos
{
    content_pos(size_t line, std::vector<std::string>* text)
        : line(line)
        , text(text)
        , defined(false) {};
    content_pos()
        : line(0)
        , text(nullptr)
        , defined(true) {};
    // line where the completion item contents start
    size_t line;
    // completion item actual contents
    std::vector<std::string>* text;
    // whether the contents were defined or not
    bool defined;
};

// representation of completion item based on LSP
class completion_item_s
{
    // contents of the item
    std::vector<std::string> content;
    // meta values, later resolved if needed ad hoc
    content_pos content_meta;

public:
    // constructor in case the contents should be resolved later in lazy style
    completion_item_s(std::string label, std::string detail, std::string insert_text, content_pos contents);
    // contents directly passed via the contrusctor
    completion_item_s(
        std::string label, std::string detail, std::string insert_text, std::vector<std::string> contents);

    std::vector<std::string> get_contents() const;
    // helper function, recreates the content vector to content string
    void implode_contents();
    // several features of completion item from LSP
    std::string label;
    std::string detail;
    std::string insert_text;
    bool deprecated = false;
    size_t kind = 1;
    // content in one string
    std::string content_string;
};

// general representation of a symbol definition
struct definition
{
    // created from lsp symbol with no addictional info
    inline definition(const lsp_symbol& sym)
        : definition(sym.name, sym.symbol_range.file, sym.symbol_range.r)
    {}

    // empty definition
    inline definition(const std::string* empty_string)
        : name(empty_string)
        , file_name(empty_string)
    {}

    inline definition(const std::string* name, const std::string* file_name, range definition_range)
        : name(name)
        , file_name(file_name)
        , definition_range(definition_range)
    {}

    // returns value of the symbol
    virtual std::vector<std::string> get_value() const;
    // custom hash function
    virtual size_t hash() const;
    bool operator==(const definition& other) const;

    // name of the symbol
    const std::string* name;
    // file where the symbol was defined
    const std::string* file_name;
    // the definition range of the symbol
    range definition_range;

    virtual ~definition() = default;
};

// derived representation of the ordinary symbol
struct ord_definition : public definition
{
    // the definition constructors
    inline ord_definition()
        : definition(nullptr) {};

    inline ord_definition(const std::string* name, const std::string* file_name, range definition_range)
        : definition(name, file_name, definition_range)
    {}

    inline ord_definition(const lsp_symbol& sym)
        : definition(sym)
    {}

    // constructor with addition of symbol value and symbol attributes
    inline ord_definition(const std::string* name,
        const std::string* file_name,
        const range& definition_range,
        const symbol_value val,
        const symbol_attributes attr)
        : definition(name, file_name, definition_range)
        , val(val)
        , attr(attr)
    {}

    virtual std::vector<std::string> get_value() const override;
    virtual size_t hash() const override;
    bool operator==(const ord_definition& other) const;

    // value of a symbol
    const symbol_value val;
    // attributes of symbol
    std::optional<const symbol_attributes> attr;
};

// variations of variable symbols: number, string, boolean or macro param
enum class var_type
{
    NUM,
    STRING,
    BOOL,
    MACRO
};

// derived representation of the variable symbol
struct var_definition : public definition
{
    // the definition constructors
    inline var_definition()
        : definition(nullptr)
        , type(var_type::BOOL)
        , scope { nullptr, 0 } {};

    inline var_definition(const lsp_symbol& sym)
        : definition(sym)
        , type(var_type::BOOL)
        , scope { nullptr, 0 }
    {}

    // constructor with addition of variable symbol type and scope of the symbol
    inline var_definition(const std::string* name,
        const std::string* file_name,
        const range& definition_range,
        var_type type,
        macro_id scope)
        : definition(name, file_name, definition_range)
        , type(type)
        , scope(scope)
    {}

    virtual std::vector<std::string> get_value() const override;
    virtual size_t hash() const override;
    bool operator==(const var_definition& other) const;

    // symbol type
    var_type type;
    // macro scope
    macro_id scope;
};

// derived representation of the sequence symbol
struct seq_definition : public definition
{
    // the definition constructors
    inline seq_definition()
        : definition(nullptr)
        , scope { nullptr, 0 } {};

    inline seq_definition(const lsp_symbol& sym)
        : definition(sym)
        , scope { nullptr, 0 }
    {}

    // constructor with addition of scope of the symbol
    inline seq_definition(
        const std::string* name, const std::string* file_name, const range& definition_range, macro_id scope)
        : definition(name, file_name, definition_range)
        , scope(scope)
    {}

    virtual std::vector<std::string> get_value() const override;
    virtual size_t hash() const override;
    bool operator==(const seq_definition& other) const;

    // macro scope
    macro_id scope;
};

// derived representation of the instruction symbol
struct instr_definition : public definition
{
    // the definition constructors
    inline instr_definition()
        : definition(nullptr)
        , version(0) {};

    inline instr_definition(const lsp_symbol& sym)
        : definition(sym)
        , version(0)
    {}

    // constructor with addition of the completion item describing the instruction and the version of the instruction
    // (macros only)
    inline instr_definition(const std::string* name,
        const std::string* file_name,
        const range definition_range,
        const completion_item_s& item,
        size_t version)
        : definition(name, file_name, definition_range)
        , item(item)
        , version(version)
    {}

    virtual std::vector<std::string> get_value() const override;
    virtual size_t hash() const override;
    bool operator==(const instr_definition& other) const;
    // (re)initialize the instruction symbol
    void init(const std::string* file, const std::string* n, const range& r);
    // clear the contents of the symbol
    void clear(const std::string* empty_string);

    // completion item providing contents about instruction
    std::optional<completion_item_s> item;
    // version of the instruction
    size_t version;
};

// ustom has function used for working with symbol definition classes
template<typename T> struct hash_function
{
    size_t operator()(const T& symbol) const { return symbol.hash(); }
};

// map of definitions to the vector of their occurences
template<class T> using definitions = std::unordered_map<T, std::vector<occurence>, hash_function<T>>;

// lsp context included in hlasm context
struct lsp_context
{
    // symbol definitions maps
    definitions<seq_definition> seq_symbols;
    definitions<var_definition> var_symbols;
    definitions<ord_definition> ord_symbols;
    definitions<instr_definition> instructions;

    // deferred symbols, used in lsp info processor
    std::vector<seq_definition> deferred_seqs;
    std::vector<ord_definition> deferred_ord_defs;
    std::vector<std::pair<ord_definition, bool>> deferred_ord_occs;
    // stack of macro calls as they are parsed
    std::stack<macro_id> parser_macro_stack;
    instr_definition deferred_macro_statement;
    // whether the copy instruction was used
    bool copy = false;
    // whether the lsp context was initialized
    bool initialized = false;
    // vecotr of all instructions with their values for completion request
    std::vector<completion_item_s> all_instructions;

    inline lsp_context()
        : deferred_macro_statement()

    {}
};

using lsp_ctx_ptr = std::shared_ptr<lsp_context>;
} // namespace context
} // namespace parser_library
} // namespace hlasm_plugin
#endif