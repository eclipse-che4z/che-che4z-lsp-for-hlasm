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
    code_scope.h
    common_types.cpp
    common_types.h
    compressed_id.h
    copy_member.h
    hlasm_context.cpp
    hlasm_context.h
    hlasm_statement.cpp
    hlasm_statement.h
    id_index.h
    id_storage.cpp
    id_storage.h
    instruction_type.h
    literal_pool.cpp
    literal_pool.h
    macro.cpp
    macro.h
    macro_param_data.cpp
    macro_param_data.h
    opcode_generation.h
    operation_code.h
    sequence_symbol.cpp
    sequence_symbol.h
    source_context.cpp
    source_context.h
    source_snapshot.h
    special_instructions.cpp
    special_instructions.h
    statement_cache.cpp
    statement_cache.h
    using.cpp
    using.h
    well_known.h
)

add_subdirectory(ordinary_assembly)
add_subdirectory(variables)
