/*
 * Copyright (c) 2021 Broadcom.
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

#include "utils/platform.h"

#ifdef __EMSCRIPTEN__
#    include <emscripten.h>

#    include <emscripten/bind.h>
#endif

namespace hlasm_plugin::utils::platform {

bool is_windows()
{
#ifdef _WIN32
    return true;
#elif __EMSCRIPTEN__
    return false;
    // clang-format off
    static const bool windows_flag = []() { return EM_ASM_INT({ return process.platform === "win32" ? 1 : 0; }); }();
    // clang-format on
    return windows_flag;
#else
    return false;
#endif
}

} // namespace hlasm_plugin::utils::platform
