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

#ifndef HLASMPLUGIN_PARSERLIBRARY_AREAD_TIME_H
#define HLASMPLUGIN_PARSERLIBRARY_AREAD_TIME_H

#include <chrono>
#include <string>

namespace hlasm_plugin::parser_library {
std::string time_to_clockb(std::chrono::nanoseconds d);
std::string time_to_clockd(std::chrono::nanoseconds d);
} // namespace hlasm_plugin::parser_library

#endif // HLASMPLUGIN_PARSERLIBRARY_AREAD_TIME_H