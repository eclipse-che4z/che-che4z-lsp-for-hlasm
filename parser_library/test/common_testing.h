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

#ifndef HLASMPLUGIN_HLASMPARSERLIBRARY_COMMON_TESTING_H
#define HLASMPLUGIN_HLASMPARSERLIBRARY_COMMON_TESTING_H

#include <utility>

#include "antlr4-runtime.h"
#include "gmock/gmock.h"

#include "analyzer.h"
#include "ebcdic_encoding.h"
#include "hlasmparser.h"
#include "lexing/input_source.h"
#include "lexing/token_stream.h"
#include "processing/context_manager.h"
#include "processing/instruction_sets/macro_processor.h"


using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::parser_library::processing;
using namespace hlasm_plugin::parser_library::expressions;

const size_t size_t_zero = static_cast<size_t>(0);

inline std::pair<bool, antlr4::ParserRuleContext*> try_parse_sll(
    hlasm_plugin::parser_library::parsing::hlasmparser& h_parser)
{
    h_parser.getInterpreter<antlr4::atn::ParserATNSimulator>()->setPredictionMode(
        antlr4::atn::PredictionMode::SLL); // try with simpler/faster SLL(*)
    // we don't want error messages or recovery during first try
    h_parser.removeErrorListeners();
    h_parser.setErrorHandler(std::make_shared<antlr4::BailErrorStrategy>());
    try
    {
        auto tree = h_parser.program();
        // if we get here, there was no syntax error and SLL(*) was enough;
        // there is no need to try full LL(*)
        return { true, tree };
    }
    catch (antlr4::RuntimeException&)
    {
        std::cout << "SLL FAILURE" << std::endl;

        auto tokens = h_parser.getTokenStream();
        std::cout << tokens->get(tokens->index())->getLine() << std::endl;
        // The BailErrorStrategy wraps the RecognitionExceptions in
        // RuntimeExceptions so we have to make sure we're detecting
        // a true RecognitionException not some other kind
        dynamic_cast<antlr4::BufferedTokenStream*>(tokens)->reset(); // rewind input stream
        // back to standard listeners/handlers
        h_parser.addErrorListener(&antlr4::ConsoleErrorListener::INSTANCE);
        h_parser.setErrorHandler(std::make_shared<antlr4::DefaultErrorStrategy>());
        h_parser.getInterpreter<antlr4::atn::ParserATNSimulator>()->setPredictionMode(
            antlr4::atn::PredictionMode::LL); // try full LL(*)
        auto tree = h_parser.program();
        return { false, tree };
    }
}
#endif