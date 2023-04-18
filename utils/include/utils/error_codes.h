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


#ifndef HLASMPLUGIN_UTILS_ERROR_CODES_H
#define HLASMPLUGIN_UTILS_ERROR_CODES_H


namespace hlasm_plugin::utils::error {

struct error_code
{
    int code;
    const char* msg;
};

namespace lsp {
constexpr int internal_error = -32603;
constexpr error_code request_canceled { -32800, "Canceled" };
constexpr error_code request_failed { -32803, "Unknown reason" };
constexpr error_code removing_workspace { -32803, "Workspace removal in progress" };
} // namespace lsp

constexpr error_code not_found { 0, "Not found" };

constexpr error_code allocation { -1, "Allocation failed" };
// constexpr error_code provide { -2, "Exception thrown while providing result" };
constexpr error_code invalid_json { -100, "Invalid JSON content" };
constexpr error_code message_send { -101, "Error occured while sending a message" };
constexpr error_code invalid_conf_response { -102, "Invalid response to 'workspace/configuration'" };
constexpr error_code invalid_request { -103, "Invalid request" };


} // namespace hlasm_plugin::utils::error

#endif
