

#include "document_symbol_item.h"

namespace hlasm_plugin::parser_library::lsp {

document_symbol_item_s::document_symbol_item_s(hlasm_plugin::parser_library::context::id_index name,
    document_symbol_kind kind,
    range symbol_range,
    range symbol_selection_range)
    : name(name)
    , kind(kind)
    , symbol_range(symbol_range)
    , symbol_selection_range(symbol_selection_range)
{}
document_symbol_item_s::document_symbol_item_s(
    hlasm_plugin::parser_library::context::id_index name, document_symbol_kind kind, range symbol_range)
    : name(name)
    , kind(kind)
    , symbol_range(symbol_range)
    , symbol_selection_range(symbol_range)
{}
document_symbol_item_s::document_symbol_item_s(hlasm_plugin::parser_library::context::id_index name,
    document_symbol_kind kind,
    range symbol_range,
    document_symbol_list_s children)
    : name(name)
    , kind(kind)
    , symbol_range(symbol_range)
    , symbol_selection_range(symbol_range)
    , children(children)
{}

bool operator==(const document_symbol_item_s& lhs, const document_symbol_item_s& rhs)
{
    if (lhs.name == nullptr || rhs.name == nullptr)
    {
        return false;
    }
    return *(lhs.name) == *(rhs.name) && lhs.kind == rhs.kind && lhs.symbol_range == rhs.symbol_range
        && lhs.symbol_selection_range == rhs.symbol_selection_range;
}

} // namespace hlasm_plugin::parser_library::lsp