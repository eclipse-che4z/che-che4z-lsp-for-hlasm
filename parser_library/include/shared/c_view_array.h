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

#ifndef HLASMPLUGIN_PARSERLIBRARY_C_VIEW_ARRAY_H	
#define HLASMPLUGIN_PARSERLIBRARY_C_VIEW_ARRAY_H	

namespace hlasm_plugin::parser_library {

//Provides pimpl for arrays. The returned item can be
//converted to its exported representation in implementation
//of item. c_type is the exported type, impl is its 
//implementation.
template<typename c_type, typename impl>
class c_view_array
{
public:
	c_view_array(const impl* data, size_t size) :
		data_(data), size_(size) {}

	//needs to be specialized for every use	
	c_type item(size_t index);
	size_t size()
	{
		return size_;
	}
private:
	const impl* data_;
	size_t size_;
};

}

#endif // !HLASMPLUGIN_PARSERLIBRARY_C_VIEW_ARRAY_H
