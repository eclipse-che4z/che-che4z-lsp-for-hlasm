

#include "document_symbol_item.h"

namespace hlasm_plugin::parser_library::lsp {

document_symbol_item_s::document_symbol_item_s(std::string name, document_symbol_kind kind, range symbol_range)
    : name(name)
    , kind(kind)
    , symbol_range(symbol_range)
    , symbol_selection_range(symbol_range)
{}
document_symbol_item_s::document_symbol_item_s(
    std::string name, document_symbol_kind kind, range symbol_range, document_symbol_list_s children)
    : name(name)
    , kind(kind)
    , symbol_range(symbol_range)
    , symbol_selection_range(symbol_range)
    , children(children)
{}

bool is_permutation_with_permutations(const document_symbol_list_s& lhs, const document_symbol_list_s& rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    for (auto& item : lhs)
    {
        auto i = std::find(rhs.begin(), rhs.end(), item);
        if (i == rhs.end())
        {
            return false;
        }
        if (!is_permutation_with_permutations(item.children, i->children))
        {
            return false;
        }
    }
    return true;
}

document_symbol_list_s::iterator document_symbol_no_children_find(
    document_symbol_list_s::iterator begin, document_symbol_list_s::iterator end, const document_symbol_item_s& item)
{
    while (begin != end)
    {
        if (item.name == begin->name
            && item.kind == begin->kind && item.symbol_range == begin->symbol_range
            && item.symbol_selection_range == begin->symbol_selection_range)
        {
            return begin;
        }
        begin++;
    }
    return end;
}

bool operator==(const document_symbol_item_s& lhs, const document_symbol_item_s& rhs)
{
    return lhs.name == rhs.name
        && lhs.kind == rhs.kind && lhs.symbol_range == rhs.symbol_range
        && lhs.symbol_selection_range == rhs.symbol_selection_range
        && is_permutation_with_permutations(lhs.children, rhs.children);
}

} // namespace hlasm_plugin::parser_library::lsp