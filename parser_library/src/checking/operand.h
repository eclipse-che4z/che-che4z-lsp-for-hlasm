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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CHECKING_OPERAND_H
#define HLASMPLUGIN_PARSERLIBRARY_CHECKING_OPERAND_H

#include <memory>
#include "shared/range.h"

namespace hlasm_plugin::parser_library::checking
{

class operand
{
public:
	operand() = default;
	operand(range operand_range) : operand_range(operand_range) {}
	range operand_range;

	virtual ~operand() = default;
};

using check_op_ptr = std::unique_ptr<operand>;

class asm_operand : public virtual operand
{
public:
	asm_operand() = default;
	virtual ~asm_operand() = default;
};


}

#endif
