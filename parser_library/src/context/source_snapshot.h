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

#ifndef CONTEXT_SOURCE_SNAPSHOT_H
#define CONTEXT_SOURCE_SNAPSHOT_H

#include "id_storage.h"
#include "range.h"

#include <vector>

namespace hlasm_plugin {
namespace parser_library {
namespace context {

//helper structure representing position in source file
struct source_position
{
	//line in the file
	size_t file_line;
	//character offset in the file
	size_t file_offset;

	source_position(size_t file_line = 0, size_t file_offset = 0) 
		:file_line(file_line), file_offset(file_offset) {}

	bool operator==(const source_position& oth) const
	{ 
		return file_line == oth.file_line && file_offset == oth.file_offset;
	}
};

//helper structure representing a copy member invocation
struct copy_frame 
{
	id_index copy_member; 
	int statement_offset;

	copy_frame(id_index copy_member, int statement_offset)
		:copy_member(copy_member), statement_offset(statement_offset) {}

	bool operator==(const copy_frame& oth) const 
	{ 
		return copy_member == oth.copy_member && statement_offset == oth.statement_offset; 
	}
};

//snapshot of a source_context structure
struct source_snapshot
{
	location instruction;
	size_t begin_index;
	size_t end_index;
	size_t end_line;
	std::vector<copy_frame> copy_frames;

	bool operator==(const source_snapshot& oth) const
	{
		if(!(end_line == oth.end_line && begin_index == oth.begin_index
			&& end_index == oth.end_index && copy_frames.size() == oth.copy_frames.size()))
			return false;

		for (size_t i = 0; i < copy_frames.size(); ++i)
			if (!(copy_frames[i] == oth.copy_frames[i]))
				return false;

		return true;
	}
};

}
}
}
#endif
