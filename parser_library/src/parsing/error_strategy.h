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

#ifndef HLASMPLUGIN_PARSERLIBRARY_ERROR_STRATEGY_H
#define HLASMPLUGIN_PARSERLIBRARY_ERROR_STRATEGY_H

#include "DefaultErrorStrategy.h"

namespace hlasm_plugin::parser_library::parsing {

// Overrides default ANTLR error strategy, so it returns our specialized
// tokens instead of ANTLR abstract tokens. The rest of implementation is
// copied from antlr4::DefaultErrorStrategy.
class error_strategy final : public antlr4::DefaultErrorStrategy
{
    bool m_error_reported = false;
    bool m_lookahead_recovery = false;

    void reset(antlr4::Parser* recognizer) override;
    void reportError(antlr4::Parser* recognizer, const antlr4::RecognitionException& e) override;
    antlr4::Token* getMissingSymbol(antlr4::Parser*) override;
    antlr4::Token* singleTokenDeletion(antlr4::Parser*) override { return nullptr; }
    bool singleTokenInsertion(antlr4::Parser*) override { return false; }
    void recover(antlr4::Parser* recognizer, std::exception_ptr e) override;
    antlr4::Token* recoverInline(antlr4::Parser* recognizer) override;

public:
    bool error_reported() const { return m_error_reported; }

    void enable_lookahead_recovery();
    void disable_lookahead_recovery();
};

} // namespace hlasm_plugin::parser_library::parsing


#endif
