/*
 * Copyright (c) 2024 Broadcom.
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

#ifndef HLASMPLUGIN_LANGUAGESERVER_ENCODINGS_H
#define HLASMPLUGIN_LANGUAGESERVER_ENCODINGS_H

#include <array>
#include <limits>

namespace hlasm_plugin::language_server {
enum class encodings;
const std::array<char16_t, (1 << std::numeric_limits<unsigned char>::digits)>* get_input_encoding(encodings);
} // namespace hlasm_plugin::language_server

#endif
