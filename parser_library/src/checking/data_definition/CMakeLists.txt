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
    data_def_fields.h
    data_def_type_base.cpp
    data_def_type_base.h
    data_def_types.h
    data_def_types_address.cpp
    data_def_types_fixed_point.cpp
    data_def_types_floating_point.cpp
    data_def_types_string.cpp
    data_definition_operand.h
    data_instruction.cpp
)

