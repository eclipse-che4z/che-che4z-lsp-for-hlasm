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
    asm_processor.cpp
    asm_processor.h
    ca_processor.cpp
    ca_processor.h
    data_def_postponed_statement.cpp
    data_def_postponed_statement.h
    instruction_processor.cpp
    instruction_processor.h
    low_language_processor.cpp
    low_language_processor.h
    mach_processor.cpp
    mach_processor.h
    macro_processor.cpp
    macro_processor.h
    postponed_statement_impl.h
)

