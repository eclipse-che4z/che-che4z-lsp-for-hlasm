#ifndef CONTEXT_SOURCE_SNAPSHOT_H
#define CONTEXT_SOURCE_SNAPSHOT_H

#include "id_storage.h"
#include "shared/range.h"

#include <vector>

namespace hlasm_plugin {
namespace parser_library {
namespace context {

struct source_position
{
	size_t file_line;
	size_t file_offset;

	source_position(size_t file_line = 0, size_t file_offset = 0) 
		:file_line(file_line), file_offset(file_offset) {}

	bool operator==(const source_position& oth) const
	{ 
		return file_line == oth.file_line && file_offset == oth.file_offset;
	}
};

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
