/*
 * Copyright (c) 2026 Broadcom.
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

#ifndef HLASMPLUGIN_LANGUAGESERVER_PSEUDO_CONVERTORS_H
#define HLASMPLUGIN_LANGUAGESERVER_PSEUDO_CONVERTORS_H

#include "server_options.h"
#include "utils/text_convertor.h"

namespace hlasm_plugin::language_server {
const utils::text_convertor* get_text_convertor(pseudo_charsets pc) noexcept;
}

#endif
