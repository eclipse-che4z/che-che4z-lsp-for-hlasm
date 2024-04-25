/*
 * Copyright (c) 2023 Broadcom.
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

#ifndef HLASMPLUGIN_PARSERLIBRARY_DIAGNOSTIC_TOOLS_H
#define HLASMPLUGIN_PARSERLIBRARY_DIAGNOSTIC_TOOLS_H

#include "context/source_context.h"

namespace hlasm_plugin::parser_library {
struct diagnostic_op;
struct diagnostic;

diagnostic add_stack_details(diagnostic_op diagnostic, context::processing_stack_t stack);

} // namespace hlasm_plugin::parser_library


#endif
