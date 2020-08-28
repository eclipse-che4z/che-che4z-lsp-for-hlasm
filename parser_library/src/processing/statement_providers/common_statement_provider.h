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

#ifndef PROCESSING_COMMON_STATEMENT_PROVIDER_H
#define PROCESSING_COMMON_STATEMENT_PROVIDER_H

#include "context/hlasm_context.h"
#include "processing/statement_fields_parser.h"
#include "statement_provider.h"
#include "context/cached_statement.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

// common class for more complicated statement providers
class common_statement_provider : public statement_provider
{
public:
    common_statement_provider(
        const statement_provider_kind kind, context::hlasm_context& hlasm_ctx, statement_fields_parser& parser);

    virtual void process_next(statement_processor& processor) override;

protected:
    context::hlasm_context& hlasm_ctx;
    statement_fields_parser& parser;

    void preprocess_deferred(statement_processor& processor, context::cached_statement_storage& cache);

    virtual context::cached_statement_storage* get_next() = 0;
};

} // namespace processing
} // namespace parser_library
} // namespace hlasm_plugin
#endif
