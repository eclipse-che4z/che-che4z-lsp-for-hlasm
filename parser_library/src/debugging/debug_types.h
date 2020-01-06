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

#ifndef HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_STACK_FRAME_H
#define HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_STACK_FRAME_H

#include <string>

#include "shared/protocol.h"
#include "variable.h"

namespace hlasm_plugin::parser_library::debugging
{

struct source
{
	source(std::string path) : path(std::move(path)) { }
	std::string path;
	bool operator==(const source& oth) const
	{
		return path == oth.path;
	}
};

struct stack_frame
{
	stack_frame(position_t begin_line, position_t end_line, uint32_t id, std::string name, source source) : begin_line(begin_line), end_line(end_line), id(id), name(std::move(name)), frame_source(std::move(source)) { }
	position_t begin_line;
	position_t end_line;
	uint32_t id;
	std::string name;
	source frame_source;
	bool operator==(const stack_frame& oth) const
	{
		return begin_line == oth.begin_line && end_line == oth.end_line && name == oth.name && frame_source == oth.frame_source;
	}
};

struct breakpoints
{
	breakpoints(source source, std::vector<breakpoint> breakpoints) :
		bps_source(std::move(source)), points(std::move(breakpoints)) {}
	source bps_source;
	std::vector<breakpoint> points;
};


struct scope
{
	scope(std::string name, var_reference_t ref, source source) : name(std::move(name)), scope_source(std::move(source)), var_reference(ref) {}
	std::string name;
	source scope_source;
	var_reference_t var_reference;
};

}

#endif // !HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_STACK_FRAME_H
