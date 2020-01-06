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

#ifndef PROCESSING_STATEMENT_PROVIDER_H
#define PROCESSING_STATEMENT_PROVIDER_H

#include "statement_processors/statement_processor.h"
#include "statement_provider_kind.h"


namespace hlasm_plugin {
namespace parser_library {
namespace processing {

class statement_provider;
using provider_ptr = std::unique_ptr<statement_provider>;

//interface for statement providers
//till they are finished they provide statements to statement processors
class statement_provider
{
public:
	const statement_provider_kind kind;

	statement_provider(const statement_provider_kind kind) : kind(kind) {}

	//processes next statement with help of a processor
	virtual void process_next(statement_processor& processor) = 0;

	//checks whether provider has finished
	virtual bool finished() const = 0;

	virtual ~statement_provider() = default;
};

}
}
}
#endif
