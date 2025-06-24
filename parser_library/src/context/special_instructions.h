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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CONTEXT_SPECIAL_INSTRUCTIONS_H
#define HLASMPLUGIN_PARSERLIBRARY_CONTEXT_SPECIAL_INSTRUCTIONS_H

namespace hlasm_plugin::parser_library::context {
class id_index;

bool instruction_resolved_during_macro_parsing(id_index name);

} // namespace hlasm_plugin::parser_library::context
#endif
