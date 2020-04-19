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

#include "context/hlasm_context.h"
#include "processing/attribute_provider.h"
#include "workspaces/parse_lib_provider.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

// structure holding required objects to correcly perform evaluation of expressions
struct evaluation_context
{
    context::hlasm_context& hlasm_ctx;
    processing::attribute_provider& attr_provider;
    workspaces::parse_lib_provider& lib_provider;
};

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin

#endif
