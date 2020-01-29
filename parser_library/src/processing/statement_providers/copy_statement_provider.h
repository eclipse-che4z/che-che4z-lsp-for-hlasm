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

#ifndef PROCESSING_COPY_STATEMENT_PROVIDER_H
#define PROCESSING_COPY_STATEMENT_PROVIDER_H

#include "common_statement_provider.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//statement provider providing statements of copy members
class copy_statement_provider : public common_statement_provider
{
public:
	copy_statement_provider(context::hlasm_context& hlasm_ctx, statement_fields_parser& parser);

	virtual void process_next(statement_processor& processor) override;

	virtual bool finished() const override;
};

}
}
}
#endif
