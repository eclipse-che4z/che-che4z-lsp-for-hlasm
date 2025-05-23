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
    branching_provider.h
    error_statement.cpp
    error_statement.h
    handler_map.h
    op_code.cpp
    op_code.h
    opencode_provider.cpp
    opencode_provider.h
    preprocessor.cpp
    preprocessor.h
    processing_manager.cpp
    processing_manager.h
    processing_state_listener.h
    statement.h
    statement_fields_parser.cpp
    statement_fields_parser.h
)

add_subdirectory(instruction_sets)
add_subdirectory(preprocessors)
add_subdirectory(statement_analyzers)
add_subdirectory(statement_processors)
add_subdirectory(statement_providers)
