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

#ifndef HLASMPLUGIN_PARSER_HLASM_EVALUATION_CONTEXT_H
#define HLASMPLUGIN_PARSER_HLASM_EVALUATION_CONTEXT_H

#include "../processing/attribute_provider.h"
#include "../context/hlasm_context.h"
#include "../parse_lib_provider.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

struct evaluation_context
{
	context::hlasm_context& hlasm_ctx;
	processing::attribute_provider& attr_provider;
	parse_lib_provider& lib_provider;
};

}
}
}

#endif
