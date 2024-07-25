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
    set_target_properties(library_fuzzer PROPERTIES LINK_FLAGS "-fsanitize=fuzzer")
else()
    add_library(library_fuzzer STATIC)
endif()

target_sources(library_fuzzer PRIVATE
    fuzzer.cpp
)

configure_file(
            fuzzer.dict
            ${CMAKE_BINARY_DIR}/bin/fuzzer.dict
            COPYONLY)

target_include_directories(library_fuzzer
    PRIVATE
    ../src
)
target_link_libraries(library_fuzzer PRIVATE parser_library)
target_link_libraries(library_fuzzer PRIVATE Threads::Threads)
target_link_libraries(library_fuzzer PRIVATE std::filesystem)
target_link_libraries(library_fuzzer PRIVATE nlohmann_json::nlohmann_json)
