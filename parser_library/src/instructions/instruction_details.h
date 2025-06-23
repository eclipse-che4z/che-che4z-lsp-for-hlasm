/*
 * Copyright (c) 2023 Broadcom.
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

#ifndef DEFINE_MACH_FORMAT
#    define DEFINE_MACH_FORMAT(...)
#endif
#ifndef DEFINE_INSTRUCTION_FORMAT
#    define DEFINE_INSTRUCTION_FORMAT(...)
#endif
#ifndef DEFINE_CC_SET
#    define DEFINE_CC_SET(...)
#endif
#ifndef DEFINE_INSTRUCTION
#    define DEFINE_INSTRUCTION(...)
#endif
#ifndef DEFINE_MNEMONIC
#    define DEFINE_MNEMONIC(...)
#endif
#ifndef DEFINE_ASM_INSTRUCTION
#    define DEFINE_ASM_INSTRUCTION(...)
#endif
#include "instruction_details.thh"
#undef DEFINE_MACH_FORMAT
#undef DEFINE_INSTRUCTION_FORMAT
#undef DEFINE_CC_SET
#undef DEFINE_INSTRUCTION
#undef DEFINE_MNEMONIC
#undef DEFINE_ASM_INSTRUCTION
