# Copyright (c) 2023 Broadcom.
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

function(generate_emscripten_node_runner target)
    if (NOT EMSCRIPTEN)
        return()
    endif()
    if (NOT ${CMAKE_EXECUTABLE_SUFFIX_CXX} STREQUAL ".mjs")
        return()
    endif()
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -D "OUTPUT_FILE=$<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_PREFIX:${target}>$<TARGET_FILE_BASE_NAME:${target}>.js" -D "INPUT_FILE=$<TARGET_FILE_NAME:${target}>" -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/emscripten_node_runner_generator.cmake"
    )
endfunction()
