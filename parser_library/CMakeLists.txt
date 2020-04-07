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

 set(GRAMMAR_SRC
	 ${PROJECT_SOURCE_DIR}/src/grammar/assembler_operand_rules.g4
	 ${PROJECT_SOURCE_DIR}/src/grammar/ca_expr_rules.g4
	 ${PROJECT_SOURCE_DIR}/src/grammar/ca_operand_rules.g4
	 ${PROJECT_SOURCE_DIR}/src/grammar/hlasmparser.g4
	 ${PROJECT_SOURCE_DIR}/src/grammar/instruction_field_rules.g4
	 ${PROJECT_SOURCE_DIR}/src/grammar/label_field_rules.g4
	 ${PROJECT_SOURCE_DIR}/src/grammar/lookahead_rules.g4
	 ${PROJECT_SOURCE_DIR}/src/grammar/machine_expr_rules.g4
	 ${PROJECT_SOURCE_DIR}/src/grammar/machine_operand_rules.g4
	 ${PROJECT_SOURCE_DIR}/src/grammar/macro_operand_rules.g4
	 ${PROJECT_SOURCE_DIR}/src/grammar/model_operand_rules.g4
	 ${PROJECT_SOURCE_DIR}/src/grammar/operand_field_rules.g4
	 ${PROJECT_SOURCE_DIR}/src/grammar/data_def_rules.g4
	 ${PROJECT_SOURCE_DIR}/src/grammar/deferred_operand_rules.g4
 )

include(tag_generated_files.cmake)

add_custom_target(GenerateParser DEPENDS ${GENERATED_SRC})
add_custom_command(OUTPUT ${GENERATED_SRC}
   COMMAND 
   ${CMAKE_COMMAND} -E make_directory ${GENERATED_FOLDER}
   COMMAND
   "${Java_JAVA_EXECUTABLE}" -jar ${ANTLR_JAR_LOCATION} -Werror -Dlanguage=Cpp -lib ${PROJECT_SOURCE_DIR}/src/grammar/ -listener -visitor -o ${GENERATED_FOLDER}/ -package hlasm_plugin::parser_library::generated hlasmparser.g4
   WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/src/grammar/"
   DEPENDS antlr4jar ${GRAMMAR_SRC} ${PROJECT_SOURCE_DIR}/src/grammar/lex.tokens
   )

set(LIB_SRC
    ${PROJECT_SOURCE_DIR}/src/context/common_types.cpp
    ${PROJECT_SOURCE_DIR}/src/context/hlasm_context.cpp
	${PROJECT_SOURCE_DIR}/src/context/lsp_context.cpp
    ${PROJECT_SOURCE_DIR}/src/context/macro.cpp
    ${PROJECT_SOURCE_DIR}/src/context/macro_param_data.cpp
    ${PROJECT_SOURCE_DIR}/src/context/sequence_symbol.cpp
    ${PROJECT_SOURCE_DIR}/src/context/variables/variable.cpp
    ${PROJECT_SOURCE_DIR}/src/context/variables/system_variable.cpp
    ${PROJECT_SOURCE_DIR}/src/context/variables/macro_param.cpp
    ${PROJECT_SOURCE_DIR}/src/context/variables/set_symbol.cpp
    ${PROJECT_SOURCE_DIR}/src/context/instruction.cpp
    ${PROJECT_SOURCE_DIR}/src/context/id_storage.cpp
    ${PROJECT_SOURCE_DIR}/src/context/processing_context.cpp
    ${PROJECT_SOURCE_DIR}/src/context/cached_statement.cpp
    ${PROJECT_SOURCE_DIR}/src/context/ordinary_assembly/location_counter.cpp
    ${PROJECT_SOURCE_DIR}/src/context/ordinary_assembly/location_counter_data.cpp
    ${PROJECT_SOURCE_DIR}/src/context/ordinary_assembly/ordinary_assembly_context.cpp
    ${PROJECT_SOURCE_DIR}/src/context/ordinary_assembly/symbol_dependency_tables.cpp
    ${PROJECT_SOURCE_DIR}/src/context/ordinary_assembly/section.cpp
    ${PROJECT_SOURCE_DIR}/src/context/ordinary_assembly/symbol.cpp
    ${PROJECT_SOURCE_DIR}/src/context/ordinary_assembly/symbol_attributes.cpp
    ${PROJECT_SOURCE_DIR}/src/context/ordinary_assembly/address_resolver.cpp
    ${PROJECT_SOURCE_DIR}/src/context/ordinary_assembly/dependency_collector.cpp
    ${PROJECT_SOURCE_DIR}/src/context/ordinary_assembly/address.cpp
    ${PROJECT_SOURCE_DIR}/src/context/ordinary_assembly/dependant.cpp
    ${PROJECT_SOURCE_DIR}/src/context/hlasm_statement.cpp
    ${PROJECT_SOURCE_DIR}/src/processing/context_manager.cpp
    ${PROJECT_SOURCE_DIR}/src/processing/statement_processors/lookahead_processor.cpp
    ${PROJECT_SOURCE_DIR}/src/processing/statement_providers/common_statement_provider.cpp
    ${PROJECT_SOURCE_DIR}/src/processing/statement_providers/copy_statement_provider.cpp
    ${PROJECT_SOURCE_DIR}/src/processing/statement_providers/macro_statement_provider.cpp
    ${PROJECT_SOURCE_DIR}/src/processing/statement_processors/macrodef_processor.cpp
    ${PROJECT_SOURCE_DIR}/src/processing/statement_processors/ordinary_processor.cpp
    ${PROJECT_SOURCE_DIR}/src/processing/statement_processors/copy_processor.cpp
    ${PROJECT_SOURCE_DIR}/src/processing/statement_processors/empty_processor.cpp
    ${PROJECT_SOURCE_DIR}/src/processing/processing_manager.cpp
    ${PROJECT_SOURCE_DIR}/src/processing/instruction_sets/asm_processor.cpp
    ${PROJECT_SOURCE_DIR}/src/processing/instruction_sets/ca_processor.cpp
    ${PROJECT_SOURCE_DIR}/src/processing/instruction_sets/low_language_processor.cpp
    ${PROJECT_SOURCE_DIR}/src/processing/instruction_sets/mach_processor.cpp
    ${PROJECT_SOURCE_DIR}/src/processing/instruction_sets/macro_processor.cpp
	${PROJECT_SOURCE_DIR}/src/parser_tools.cpp
    ${PROJECT_SOURCE_DIR}/src/ebcdic_encoding.cpp
    ${PROJECT_SOURCE_DIR}/src/expressions/keyword_expression.cpp
    ${PROJECT_SOURCE_DIR}/src/expressions/arithmetic_expression.cpp
    ${PROJECT_SOURCE_DIR}/src/expressions/logic_expression.cpp
    ${PROJECT_SOURCE_DIR}/src/expressions/character_expression.cpp
    ${PROJECT_SOURCE_DIR}/src/expressions/expression.cpp
    ${PROJECT_SOURCE_DIR}/src/expressions/mach_expression.cpp
    ${PROJECT_SOURCE_DIR}/src/expressions/mach_expr_term.cpp
    ${PROJECT_SOURCE_DIR}/src/expressions/nominal_value.cpp
    ${PROJECT_SOURCE_DIR}/src/expressions/data_definition.cpp
    ${PROJECT_SOURCE_DIR}/src/semantics/concatenation.cpp
    ${PROJECT_SOURCE_DIR}/src/semantics/operand.cpp
	${PROJECT_SOURCE_DIR}/src/semantics/range_provider.cpp
    ${PROJECT_SOURCE_DIR}/src/expressions/visitors/expression_analyzer.cpp
    ${PROJECT_SOURCE_DIR}/src/expressions/visitors/expression_evaluator.cpp
    ${PROJECT_SOURCE_DIR}/src/semantics/lsp_info_processor.cpp
    ${PROJECT_SOURCE_DIR}/src/workspace.cpp
    ${PROJECT_SOURCE_DIR}/src/wildcard.cpp
    ${PROJECT_SOURCE_DIR}/src/file_manager_impl.cpp
    ${PROJECT_SOURCE_DIR}/src/file_impl.cpp
    ${PROJECT_SOURCE_DIR}/src/library.cpp
	${PROJECT_SOURCE_DIR}/src/processor_file_impl.cpp
	${PROJECT_SOURCE_DIR}/src/parser_error_listener.cpp
	${PROJECT_SOURCE_DIR}/src/parser_error_listener_ctx.cpp
	${PROJECT_SOURCE_DIR}/src/parser_impl.cpp
	${PROJECT_SOURCE_DIR}/src/diagnostic.cpp
	${PROJECT_SOURCE_DIR}/src/diagnostic_collector.cpp
	${PROJECT_SOURCE_DIR}/src/checking/instr_operand.cpp
    ${PROJECT_SOURCE_DIR}/src/checking/asm_instr_check.cpp
	${PROJECT_SOURCE_DIR}/src/checking/data_definition/data_instruction.cpp
	${PROJECT_SOURCE_DIR}/src/checking/data_definition/data_def_type_base.cpp
	${PROJECT_SOURCE_DIR}/src/checking/data_definition/data_def_types_address.cpp
	${PROJECT_SOURCE_DIR}/src/checking/data_definition/data_def_types_fixed_point.cpp
	${PROJECT_SOURCE_DIR}/src/checking/data_definition/data_def_types_floating_point.cpp
	${PROJECT_SOURCE_DIR}/src/checking/data_definition/data_def_types_string.cpp
	${PROJECT_SOURCE_DIR}/src/checking/data_definition/data_definition_operand.cpp
	${PROJECT_SOURCE_DIR}/src/checking/instruction_checker.cpp
	${PROJECT_SOURCE_DIR}/src/checking/asm_instr_class.cpp
	${PROJECT_SOURCE_DIR}/src/analyzer.cpp
	${PROJECT_SOURCE_DIR}/src/semantics/collector.cpp
	${PROJECT_SOURCE_DIR}/src/parse_lib_provider.cpp
	${PROJECT_SOURCE_DIR}/src/debugging/debugger.cpp
	${PROJECT_SOURCE_DIR}/src/debugging/macro_param_variable.cpp
	${PROJECT_SOURCE_DIR}/src/debugging/set_symbol_variable.cpp
	${PROJECT_SOURCE_DIR}/src/debugging/ordinary_symbol_variable.cpp
	${PROJECT_SOURCE_DIR}/src/debugging/variable.cpp
	${PROJECT_SOURCE_DIR}/src/debugging/attribute_variable.cpp

	${PROJECT_SOURCE_DIR}/src/parser_library.cpp
    ${PROJECT_SOURCE_DIR}/src/lexer.cpp
    ${PROJECT_SOURCE_DIR}/src/workspace_manager.cpp
    ${PROJECT_SOURCE_DIR}/src/token.cpp
    ${PROJECT_SOURCE_DIR}/src/token_factory.cpp
    ${PROJECT_SOURCE_DIR}/src/token_stream.cpp
    ${PROJECT_SOURCE_DIR}/src/input_source.cpp
    ${PROJECT_SOURCE_DIR}/src/protocol.cpp
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
    file(GLOB TEST_SRC
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

if(BUILD_FUZZER)
	add_executable(library_fuzzer fuzzer.cpp
	)
    target_link_libraries(library_fuzzer parser_library)


    set_target_properties(library_fuzzer PROPERTIES LINK_FLAGS "-fsanitize=fuzzer")
	set_target_properties(library_fuzzer PROPERTIES COMPILE_FLAGS "-DANTLR4CPP_STATIC")
    
	configure_file(
                ${PROJECT_SOURCE_DIR}/fuzzer.dict
                ${CMAKE_BINARY_DIR}/bin/fuzzer.dict
				COPYONLY)

    if(UNIX)
	    target_link_libraries(library_fuzzer pthread)
    endif()
	target_link_libraries(library_fuzzer ${ANTLR4_RUNTIME})
	if(FILESYSTEM_LINK)
		target_link_libraries(library_fuzzer ${FILESYSTEM_LIBRARY})
    endif()
    
	add_dependencies(library_fuzzer antlr4jar json)
endif()
