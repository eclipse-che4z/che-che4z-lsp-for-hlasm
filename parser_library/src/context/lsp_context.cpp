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

#include "lsp_context.h"

using namespace hlasm_plugin::parser_library::context;

bool definition::operator==(const definition& other) const
{
	if (check_scopes || other.check_scopes)
		return name == other.name && scope == other.scope && file_name == other.file_name && version == other.version;
	else
		return name == other.name && version == other.version;
}

bool definition::operator!=(const definition& other) const
{
	return !(other == *this);
}

bool definition::operator<(const definition& other) const
{
	if (check_scopes || other.check_scopes)
		return std::tie(name, scope, file_name,version) < std::tie(other.name, other.scope, other.file_name,other.version);
	else
		return std::tie(name,version) < std::tie(other.name, other.version);
}