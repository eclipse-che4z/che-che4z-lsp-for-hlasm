# Copyright (c) 2021 Broadcom.
# The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0
#
# Contributors:
#   Broadcom, Inc. - initial API and implementation

target_sources(parser_library PRIVATE
    asm_instr_check.cpp
    asm_instr_check.h
    asm_instr_class.cpp
    asm_instr_class.h
    checker_helper.h
    data_check.h
    diagnostic_collector.cpp
    diagnostic_collector.h
    instr_operand.cpp
    instr_operand.h
    instruction_checker.cpp
    instruction_checker.h
    machine_check.cpp
    machine_check.h
    operand.h
    using_label_checker.cpp
    using_label_checker.h
)

add_subdirectory(data_definition)
