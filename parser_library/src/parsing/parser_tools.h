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


#pragma once

#include <string>

#include "antlr4-runtime.h"

namespace hlasm_plugin::parser_library::parsing {
// returns first subtree of specific type of the given tree
// will not work properly with empty rules in grammar
std::vector<antlr4::tree::ParseTree*> get_sub_trees(antlr4::tree::ParseTree* tree, size_t type);

// checks whether the given tree is of expected type
bool is_tree_type(antlr4::tree::ParseTree* tree, size_t type);

// parser_library::context::semantic_highlighting_info semantic_highlighting(antlr4::ParserRuleContext * tree,
// std::map<std::string,size_t> rules); indented representation of parsed tree
class useful_tree
{
public:
    useful_tree(std::vector<antlr4::ParserRuleContext*> _tree,
        const antlr4::dfa::Vocabulary& _vocab,
        const std::vector<std::string>& _rules);

    void out_tree(std::ostream& stream);

private:
    std::vector<antlr4::ParserRuleContext*> tree_;
    const antlr4::dfa::Vocabulary& vocab_;
    const std::vector<std::string>& rules_;

    void out_tree_rec(antlr4::ParserRuleContext* tree, std::string indent, std::ostream& stream);
};

} // namespace hlasm_plugin::parser_library::parsing
