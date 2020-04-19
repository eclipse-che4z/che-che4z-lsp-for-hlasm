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

cmake_minimum_required (VERSION 3.10)
include(GenerateExportHeader)
include(GoogleTest)
project(parser_library)

SET (CMAKE_CXX_STANDARD 17)

set(GENERATED_FOLDER ${CMAKE_BINARY_DIR}/generated_parser/)
#generated grammar source files
set(GENERATED_SRC
   ${GENERATED_FOLDER}/hlasmparser.cpp
   ${GENERATED_FOLDER}/hlasmparserBaseListener.cpp
   ${GENERATED_FOLDER}/hlasmparserBaseVisitor.cpp
   ${GENERATED_FOLDER}/hlasmparserListener.cpp
   ${GENERATED_FOLDER}/hlasmparserVisitor.cpp
 )

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set_source_files_properties(${GENERATED_FOLDER}/hlasmparser.cpp PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
endif()

file(GLOB GRAMMAR_SRC
    "${PROJECT_SOURCE_DIR}/src/parsing/grammar/*.g4"
)

include(tag_generated_files.cmake)

add_custom_target(GenerateParser DEPENDS ${GENERATED_SRC})
add_custom_command(OUTPUT ${GENERATED_SRC}
   COMMAND 
   ${CMAKE_COMMAND} -E make_directory ${GENERATED_FOLDER}
   COMMAND
   "${Java_JAVA_EXECUTABLE}" -jar ${ANTLR_JAR_LOCATION} -Werror -Dlanguage=Cpp -lib ${PROJECT_SOURCE_DIR}/src/parsing/grammar/ -visitor -o ${GENERATED_FOLDER}/ -package hlasm_plugin::parser_library::parsing hlasmparser.g4
   WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/src/parsing/grammar/"
   DEPENDS antlr4jar ${GRAMMAR_SRC} ${PROJECT_SOURCE_DIR}/src/parsing/grammar/lex.tokens
   )

file(GLOB_RECURSE LIB_SRC
	"${PROJECT_SOURCE_DIR}/src/*.cpp"
)

#Generate the shared library from the library sources
add_library(parser_library
    ${LIB_SRC}
    ${GENERATED_SRC}
)
if(NOT BUILD_SHARED_LIBS)
	set_target_properties(parser_library PROPERTIES COMPILE_FLAGS "-DANTLR4CPP_STATIC")
endif()

generate_export_header(parser_library
			EXPORT_FILE_NAME ${GENERATED_FOLDER}/export/parser_library_export.h)

target_link_libraries(parser_library ${ANTLR4_RUNTIME})
if(FILESYSTEM_LINK)
	target_link_libraries(parser_library ${FILESYSTEM_LIBRARY})
endif()

target_include_directories(parser_library
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/include
		${PROJECT_SOURCE_DIR}/src
		${GENERATED_FOLDER}
		${GENERATED_FOLDER}/export
)

ADD_CUSTOM_COMMAND (TARGET parser_library POST_BUILD      # Adds a post-build event to MyTest
    COMMAND ${CMAKE_COMMAND} -E copy_directory  # which executes "cmake - E copy_if_different..."
        "${ANTLR4CPP_LIBS}"      # <--this is in-file
        "${PROJECT_BINARY_DIR}/../bin/" )  

add_dependencies(parser_library antlr4jar GenerateParser)
add_dependencies(parser_library json)

if(BUILD_TESTING)
    file(GLOB_RECURSE TEST_SRC
        "${PROJECT_SOURCE_DIR}/test/*.cpp"
    )

    if(BUILD_SHARED_LIBS) #when building shared libary, we need to compile from source,
                          #because not all classes are exported
	    add_executable(library_test
            ${TEST_SRC}
	    	${LIB_SRC}
			${GENERATED_SRC}
	    )
    else() #when building statically linked library, we can link already built parser_library.
    	add_executable(library_test ${TEST_SRC})
        target_link_libraries(library_test parser_library)
		set_target_properties(library_test PROPERTIES COMPILE_FLAGS "-DANTLR4CPP_STATIC")
	endif()

	if(MSVC)
		target_compile_options(library_test PRIVATE /bigobj)
	endif()

	target_include_directories(library_test
		PUBLIC
			${PROJECT_SOURCE_DIR}/include
			${PROJECT_SOURCE_DIR}/src
			${GENERATED_FOLDER}
			${GENERATED_FOLDER}/export
	)

	ADD_CUSTOM_COMMAND (TARGET library_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory  
        "${ANTLR4CPP_LIBS}"      
        "${PROJECT_BINARY_DIR}/../bin/" )  

	add_custom_target(library_tests_copy
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${PROJECT_SOURCE_DIR}/test/res ${CMAKE_BINARY_DIR}/bin/test/library)

	target_link_libraries(library_test gmock_main)
	target_link_libraries(library_test ${ANTLR4_RUNTIME})
	if(FILESYSTEM_LINK)
		target_link_libraries(library_test ${FILESYSTEM_LIBRARY})
    endif()
    
	add_dependencies(library_test library_tests_copy)
	add_dependencies(library_test antlr4jar)
	add_dependencies(library_test json)

    if(DISCOVER_TESTS)
	    gtest_discover_tests(library_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin DISCOVERY_TIMEOUT 30)
    endif()
endif()

add_subdirectory(fuzzer)
