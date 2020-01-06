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

#ifndef PROCESSING_PROCESSING_FORMAT_H
#define PROCESSING_PROCESSING_FORMAT_H

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

enum class processing_kind
{
	ORDINARY, LOOKAHEAD, MACRO, COPY
};

enum class processing_form
{
	MACH, ASM, MAC, CA, DAT, IGNORED, DEFERRED, UNKNOWN
};

enum class ordinary_proc_type
{
	MACH, ASM, MAC, CA, DAT, UNKNOWN
};

enum class lookahead_proc_type
{
	COPY, EQU, IGNORED
};

enum class macro_proc_type
{
	PROTO, COPY, CA, DEFERRED
};

enum class operand_occurence
{
	PRESENT,ABSENT
};

//structure respresenting in which fashion should be statement processed
struct processing_format
{
	processing_format(processing_kind kind, processing_form form, operand_occurence occurence = operand_occurence::PRESENT)
		: kind(kind), form(form), occurence(occurence) {}

	processing_kind kind;
	processing_form form;
	operand_occurence occurence;
};

}
}
}
#endif
