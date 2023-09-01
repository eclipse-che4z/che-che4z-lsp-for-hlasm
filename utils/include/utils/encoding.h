/*
 * Copyright (c) 2022 Broadcom.
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

#include <string>
#include <string_view>

namespace hlasm_plugin::utils::encoding {
// Returns percent encoded string
std::string percent_encode(std::string_view s);

// Returns percent encoded string while ignoring already encoded utf-8 chars
std::string percent_encode_and_ignore_utf8(std::string_view s);

std::string percent_decode(std::string_view s);

std::string uri_friendly_base16_encode(std::string_view s);
std::string uri_friendly_base16_decode(std::string_view s);

} // namespace hlasm_plugin::utils::encoding
