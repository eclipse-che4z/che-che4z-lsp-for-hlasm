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

#ifndef CONTEXT_LITERAL_STORAGE_H
#define CONTEXT_LITERAL_STORAGE_H

#include <unordered_set>
#include <string>

namespace hlasm_plugin{
namespace parser_library{
namespace context{


//storage for identifiers 
//changes strings of identifiers to indexes of this storage class for easier and unified work
class id_storage
{
	static const std::string empty_string_;
public:
	using const_pointer = const std::string *;
	using const_iterator = typename std::unordered_set<std::string>::const_iterator;

	//represents value of empty identifier
	static const const_pointer empty_id;

	size_t size() const;
	const_iterator begin() const;
	const_iterator end() const;
	bool empty() const;

	const_pointer find(std::string val) const;

	const_pointer add(std::string value);

private:
	std::unordered_set<std::string> lit_;
};

using id_index = id_storage::const_pointer;

}
}
}


#endif
