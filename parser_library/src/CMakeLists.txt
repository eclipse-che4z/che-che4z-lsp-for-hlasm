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
    aread_time.cpp
    aread_time.h
    analyzer.cpp
    compiler_options.cpp
    diagnosable_ctx.cpp
    diagnosable_ctx.h
    diagnostic.cpp
    diagnostic_adder.h
    diagnostic_consumer.h
    diagnostic_op.cpp
    diagnostic_op.h
    diagnostic_tools.h
    diagnostic_tools.cpp
    document.cpp
    ebcdic_encoding.cpp
    ebcdic_encoding.h
    empty_parse_lib_provider.cpp
    empty_parse_lib_provider.h
    error_messages.h
    fade_messages.cpp
    lib_config.cpp
    library_info.h
    library_info_transitional.cpp
    library_info_transitional.h
    output_handler.h
    tagged_index.h
    virtual_file_monitor.h
    workspace_manager.cpp
)

add_subdirectory(checking)
add_subdirectory(config)
add_subdirectory(context)
add_subdirectory(debugging)
add_subdirectory(expressions)
add_subdirectory(instructions)
add_subdirectory(lexing)
add_subdirectory(lsp)
add_subdirectory(parsing)
add_subdirectory(processing)
add_subdirectory(semantics)
add_subdirectory(workspaces)
