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

#include "id_storage.h"
#include "common_types.h"

using namespace hlasm_plugin::parser_library::context;

const std::string id_storage::empty_string_("");

const id_storage::const_pointer id_storage::empty_id = &id_storage::empty_string_;

size_t id_storage::size() const { return lit_.size(); }

id_storage::const_iterator id_storage::begin() const { return lit_.begin(); }

id_storage::const_iterator id_storage::end() const { return lit_.end(); }

bool id_storage::empty() const { return lit_.empty(); }

id_storage::const_pointer id_storage::find(std::string val) const
{
	to_upper(val);

	if (val.empty())
		return empty_id;

	const_iterator tmp = lit_.find(val);

	return tmp == lit_.end() ? nullptr : &*tmp;
}

id_storage::const_pointer id_storage::add(std::string value)
{
	return value.empty() ? empty_id : &*lit_.insert(std::move(to_upper(value))).first;
}