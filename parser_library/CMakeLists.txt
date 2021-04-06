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

set(GENERATED_FOLDER ${CMAKE_BINARY_DIR}/generated_parser/)

add_library(parser_library)

add_subdirectory(src)
add_subdirectory(include)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(parser_library PUBLIC PARSER_LIBRARY_STATIC_DEFINE=1)
endif()

generate_export_header(parser_library
			EXPORT_FILE_NAME ${GENERATED_FOLDER}/export/parser_library_export.h)

target_sources(parser_library PUBLIC ${GENERATED_FOLDER}/export/parser_library_export.h)

target_include_directories(parser_library PUBLIC
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/src
	${GENERATED_FOLDER}
	${GENERATED_FOLDER}/export
)

target_link_libraries(parser_library parser_library_generated)
target_link_libraries(parser_library nlohmann_json::nlohmann_json)
target_link_libraries(parser_library ${ANTLR4_RUNTIME})
target_link_libraries(parser_library std::filesystem)
target_link_libraries(parser_library hlasm_utils)

if(BUILD_TESTING)
	add_subdirectory(test)
endif()

if(BUILD_FUZZER)
	add_subdirectory(fuzzer)
endif()
