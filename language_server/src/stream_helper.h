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

#ifndef HLASMPLUGIN_LANGUAGESERVER_STREAM_HELPER_H
#define HLASMPLUGIN_LANGUAGESERVER_STREAM_HELPER_H

#include <iosfwd>

namespace hlasm_plugin::language_server {

void imbue_stream_newline_is_space(std::ios& stream);

} // namespace hlasm_plugin::language_server

#endif
