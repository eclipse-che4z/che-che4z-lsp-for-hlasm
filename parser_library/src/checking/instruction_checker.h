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

#ifndef HLASMPLUGIN_PARSERLIBRARY_INSTRUCTION_CHECKER_H
#define HLASMPLUGIN_PARSERLIBRARY_INSTRUCTION_CHECKER_H

#include <map>

#include "asm_instr_check.h"
#include "context/instruction.h"

namespace hlasm_plugin{
namespace parser_library{
namespace checking{

//interface for unified checking
class instruction_checker
{
public:
	virtual bool check(const std::string& instruction_name, const std::vector<const operand*>& operand_vector, const range & stmt_range, const diagnostic_collector& add_diagnostic) const = 0;
};

// derived checker for assembler instructions
class assembler_checker : public instruction_checker

{
public:
	assembler_checker();
	virtual bool check(const std::string& instruction_name, const std::vector<const operand*>& operand_vector, const range& stmt_range, const diagnostic_collector& add_diagnostic) const override;
	// map of all assembler instruction maes to their representations
    static std::map <std::string, std::unique_ptr<hlasm_plugin::parser_library::checking::assembler_instruction>> assembler_instruction_map;
protected:
	void initialize_assembler_map();
};

// derived checker for machine instructions
class machine_checker : public instruction_checker
{
public:
	virtual bool check(const std::string& instruction_name, const std::vector<const operand*>& operand_vector, const range& stmt_range, const diagnostic_collector& add_diagnostic) const override;
};

}
}
}

#endif
