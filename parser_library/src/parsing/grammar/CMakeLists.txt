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
#generated grammar source files

set(GENERATED_SRC_CPP
	${GENERATED_FOLDER}/hlasmparser.cpp
)

set(GENERATED_SRC
   ${GENERATED_SRC_CPP}
   ${GENERATED_FOLDER}/hlasmparser.h
 )

file(GLOB GRAMMAR_SRC
    "*.g4"
)

#add_custom_command(OUTPUT ${GENERATED_SRC}
#   COMMAND 
#   ${CMAKE_COMMAND} -E make_directory ${GENERATED_FOLDER}
#   COMMAND
#   "${Java_JAVA_EXECUTABLE}" -jar \"${ANTLR_JAR_LOCATION}\" -Werror -Dlanguage=Cpp -lib ${PROJECT_SOURCE_DIR}/src/parsing/grammar/ -o ${GENERATED_FOLDER}/ -package hlasm_plugin::parser_library::parsing hlasmparser.g4
#   WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/src/parsing/grammar/"
#   DEPENDS antlr4jar ${GRAMMAR_SRC} lex.tokens
#)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set_source_files_properties(${GENERATED_FOLDER}/hlasmparser.cpp PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
endif()

add_library(parser_library_generated OBJECT "${GENERATED_SRC}")

target_include_directories(parser_library_generated PRIVATE
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/src
	${GENERATED_FOLDER}
	${GENERATED_FOLDER}/export
)
target_link_libraries(parser_library_generated ${ANTLR4_RUNTIME})
