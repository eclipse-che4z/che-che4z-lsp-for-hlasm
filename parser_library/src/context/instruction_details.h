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

#ifndef DEFINE_INSTRUCTION_FORMAT
#    define DEFINE_INSTRUCTION_FORMAT(...)
#endif
#ifndef DEFINE_CC_SET
#    define DEFINE_CC_SET(...)
#endif
#ifndef DEFINE_INSTRUCTION
#    define DEFINE_INSTRUCTION(...)
#endif
#include "instruction_details.thh"
#undef DEFINE_INSTRUCTION_FORMAT
#undef DEFINE_CC_SET
#undef DEFINE_INSTRUCTION
