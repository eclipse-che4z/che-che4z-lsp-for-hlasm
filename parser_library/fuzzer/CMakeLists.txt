# Copyright (c) 2019 Broadcom.
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
#

Project(fuzzer)

if(BUILD_FUZZER)
    add_executable(library_fuzzer)
    target_link_options(library_fuzzer PRIVATE -fsanitize=fuzzer)
else()
    add_library(library_fuzzer STATIC)
endif()

target_compile_features(library_fuzzer PRIVATE cxx_std_20)
target_compile_options(library_fuzzer PRIVATE ${HLASM_EXTRA_FLAGS})
set_target_properties(library_fuzzer PROPERTIES CXX_EXTENSIONS OFF)

target_sources(library_fuzzer PRIVATE
    fuzzer.cpp
)

add_custom_command(
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/fuzzer.dict
        COMMAND
            ${CMAKE_COMMAND} -E copy fuzzer.dict "${CMAKE_CURRENT_BINARY_DIR}/fuzzer.dict"
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(copy_fuzzer_dict
        ALL
        DEPENDS fuzzer.dict
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(library_fuzzer
    PRIVATE
    ../src
)
target_link_libraries(library_fuzzer PRIVATE parser_library)
target_link_libraries(library_fuzzer PRIVATE Threads::Threads)
target_link_libraries(library_fuzzer PRIVATE nlohmann_json::nlohmann_json)

target_link_options(library_fuzzer PRIVATE ${HLASM_EXTRA_LINKER_FLAGS})

add_dependencies(library_fuzzer copy_fuzzer_dict)
