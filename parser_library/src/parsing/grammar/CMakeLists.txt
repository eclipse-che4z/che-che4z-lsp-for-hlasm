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
	${GENERATED_GRAMMER_FOLDER}/hlasmparser_singleline.cpp
	${GENERATED_GRAMMER_FOLDER}/hlasmparser_multiline.cpp
)

set(GENERATED_SRC
   ${GENERATED_SRC_CPP}
   ${GENERATED_GRAMMER_FOLDER}/hlasmparser_singleline.h
   ${GENERATED_GRAMMER_FOLDER}/hlasmparser_multiline.h
 )

if(NOT USE_PRE_GENERATED_GRAMMAR)
	file(GLOB GRAMMAR_SRC
		"*.g4"
	)

	add_custom_command(OUTPUT ${GENERATED_SRC}
	   COMMAND
	   ${CMAKE_COMMAND} -E make_directory ${GENERATED_GRAMMER_FOLDER}
	   COMMAND
	   "${Java_JAVA_EXECUTABLE}" -jar ${ANTLR_JAR_LOCATION} -Werror -Dlanguage=Cpp -lib ${PROJECT_SOURCE_DIR}/src/parsing/grammar/ -o ${GENERATED_GRAMMER_FOLDER}/ -package hlasm_plugin::parser_library::parsing hlasmparser_singleline.g4
	   COMMAND
	   "${Java_JAVA_EXECUTABLE}" -jar ${ANTLR_JAR_LOCATION} -Werror -Dlanguage=Cpp -lib ${PROJECT_SOURCE_DIR}/src/parsing/grammar/ -o ${GENERATED_GRAMMER_FOLDER}/ -package hlasm_plugin::parser_library::parsing hlasmparser_multiline.g4
	   WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/src/parsing/grammar/"
	   DEPENDS antlr4jar ${GRAMMAR_SRC} lex.tokens
	)

	add_custom_target(generate_grammar
		DEPENDS
			${GENERATED_SRC}
	)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set_source_files_properties(${GENERATED_GRAMMER_FOLDER}/hlasmparser_singleline.cpp PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
	set_source_files_properties(${GENERATED_GRAMMER_FOLDER}/hlasmparser_multiline.cpp PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
endif()

add_library(parser_library_generated OBJECT "${GENERATED_SRC}")

target_include_directories(parser_library_generated PRIVATE
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/src
	${GENERATED_GRAMMER_FOLDER}
	${GENERATED_FOLDER}
)
target_link_libraries(parser_library_generated ${ANTLR4_RUNTIME})
target_link_libraries(parser_library_generated hlasm_utils)
