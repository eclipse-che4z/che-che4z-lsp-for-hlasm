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

#ifndef HLASMPLUGIN_PARSERLIBRARY_DIAGNOSTIC_COLLECTOR_H
#define HLASMPLUGIN_PARSERLIBRARY_DIAGNOSTIC_COLLECTOR_H

#include <vector>
#include "context/processing_context.h"
#include "shared/protocol.h"
#include "diagnostic.h"

namespace hlasm_plugin::parser_library {

class diagnosable_ctx;

class diagnostic_collector
{
	diagnosable_ctx* diagnoser_;
	context::processing_stack_t location_stack_;
public:
	diagnostic_collector(diagnosable_ctx* diagnoser, context::processing_stack_t location_stack);

	diagnostic_collector(diagnosable_ctx* diagnoser);

	diagnostic_collector();

	void operator()(diagnostic_op diagnostic) const;
};

}
#endif