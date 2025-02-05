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

add_library(parser_library STATIC)

target_compile_features(parser_library PUBLIC cxx_std_20)
target_compile_options(parser_library PRIVATE ${HLASM_EXTRA_FLAGS})
set_target_properties(parser_library PROPERTIES CXX_EXTENSIONS OFF)

add_subdirectory(src)
add_subdirectory(include)

set_property(TARGET parser_library PROPERTY VERIFY_INTERFACE_HEADER_SETS ON)

target_include_directories(parser_library
    PUBLIC
    include
    PRIVATE
    src
)

target_link_libraries(parser_library PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(parser_library PUBLIC hlasm_utils)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()

add_subdirectory(fuzzer)
