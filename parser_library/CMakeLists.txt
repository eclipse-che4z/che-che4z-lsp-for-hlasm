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

include(GenerateExportHeader)
include(GoogleTest)
project(parser_library)

set(GENERATED_FOLDER ${CMAKE_BINARY_DIR}/generated_sources/)
if(NOT USE_PRE_GENERATED_GRAMMAR)
    set(GENERATED_GRAMMER_FOLDER ${CMAKE_BINARY_DIR}/generated_parser/)
else()
    set(GENERATED_GRAMMER_FOLDER ${USE_PRE_GENERATED_GRAMMAR})
endif()

add_library(parser_library STATIC)

add_subdirectory(src)
add_subdirectory(include)

set_property(TARGET parser_library PROPERTY VERIFY_INTERFACE_HEADER_SETS ON)

target_include_directories(parser_library
    PUBLIC
    include
    PRIVATE
    src
    ${GENERATED_GRAMMER_FOLDER}
    ${GENERATED_FOLDER}
)

target_link_libraries(parser_library PRIVATE parser_library_generated)
target_link_libraries(parser_library PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(parser_library PRIVATE ${ANTLR4_RUNTIME})
target_link_libraries(parser_library PRIVATE std::filesystem)
target_link_libraries(parser_library PUBLIC hlasm_utils)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()

add_subdirectory(fuzzer)
