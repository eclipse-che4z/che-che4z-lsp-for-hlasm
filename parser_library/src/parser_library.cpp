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

#include <iostream>

#include "../include/shared/parser_library.h"
#include "../include/shared/token_stream.h"
#include "parser_tools.h"
#include "context/hlasm_context.h"
#include "hlasmparser.h"
#include "analyzer.h"
#include "workspace.h"
#include "sstream"



namespace hlasm_plugin {
namespace parser_library {

//Parses specified string and outputs analysis to standard out.
//Used for testing purposes.
void parser_library::parse(const std::string & src)
{
	analyzer a(src);

	auto l = new antlr4::DiagnosticErrorListener();
	a.parser().addErrorListener(l);
	a.parser().getInterpreter<antlr4::atn::ParserATNSimulator>()->setPredictionMode(antlr4::atn::PredictionMode::LL_EXACT_AMBIG_DETECTION);

	a.analyze();

	auto tree = a.parser().tree;
	auto vocab = a.parser().getVocabulary();
	for (auto && token : dynamic_cast<antlr4::BufferedTokenStream*>(a.parser().getTokenStream())->getTokens())
	{
		auto type = token->getType();
		std::cout << vocab.getSymbolicName(type);
		if (type != lexer::Tokens::EOLLN && type != lexer::Tokens::SPACE && type != lexer::Tokens::CONTINUATION && type != lexer::Tokens::IGNORED) std::cout << "---" << "\"" << token->getText() << "\"";
		std::cout << std::endl;
	}
	std::cout << l << std::endl;

	try
	{
		std::stringstream ss;
		hlasm_plugin::parser_tools::useful_tree mytree(tree, vocab, a.parser().getRuleNames());
		mytree.out_tree(ss);

		std::cout << ss.str();
	}
	catch(...) 
	{
		std::cout << "tree could not be visualized" << std::endl;
	}
	
	a.collect_diags();
	for (auto& diag : a.diags())
	{
		std::cout << diag.diag_range.start.line << ": " << diag.message << "\n";
	}
}
}
}
