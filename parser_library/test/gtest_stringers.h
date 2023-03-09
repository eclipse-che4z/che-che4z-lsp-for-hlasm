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

#ifndef HLASMPLUGIN_PARSERLIBRARY_TEST_GTEST_STRINGERS_H
#define HLASMPLUGIN_PARSERLIBRARY_TEST_GTEST_STRINGERS_H

#include <iosfwd>

namespace hlasm_plugin::parser_library {

struct position;
struct range;
struct location;
struct performance_metrics;

std::ostream& operator<<(std::ostream& stream, const position& item);

std::ostream& operator<<(std::ostream& stream, range item);

std::ostream& operator<<(std::ostream& stream, const location& item);

std::ostream& operator<<(std::ostream& stream, const performance_metrics& item);

} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::lsp {

struct symbol_occurrence;
struct completion_item_s;

std::ostream& operator<<(std::ostream& stream, const symbol_occurrence& item);

std::ostream& operator<<(std::ostream& stream, const completion_item_s& item);

} // namespace hlasm_plugin::parser_library::lsp


#endif
