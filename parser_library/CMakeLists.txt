cmake_minimum_required (VERSION 3.10)
INCLUDE(GenerateExportHeader)
include(GoogleTest)
project(parser_library)

SET (CMAKE_CXX_STANDARD 17)


# set ANTLR jar location
set(ANTLR_JAR_LOCATION 
	${ANTLR4CPP_EXTERNAL_ROOT}/src/antlr4cpp/tool/target/antlr4-4.7.1-complete.jar)

# generated grammar source files
set(GENERATED_SRC
   ${PROJECT_SOURCE_DIR}/src/generated/hlasmparser.cpp
   ${PROJECT_SOURCE_DIR}/src/generated/hlasmparserBaseListener.cpp
   ${PROJECT_SOURCE_DIR}/src/generated/hlasmparserBaseVisitor.cpp
   ${PROJECT_SOURCE_DIR}/src/generated/hlasmparserListener.cpp
   ${PROJECT_SOURCE_DIR}/src/generated/hlasmparserVisitor.cpp
 )

foreach( src_file ${GENERATED_SRC} )
      set_source_files_properties(
          ${src_file}
          PROPERTIES
          GENERATED TRUE
          )
endforeach( src_file ${GENERATED_SRC} )

add_custom_target(GenerateParser DEPENDS ${GENERATED_SRC})
add_custom_command(OUTPUT ${GENERATED_SRC}
   COMMAND 
   ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/src/generated/
   COMMAND
   "${Java_JAVA_EXECUTABLE}" -jar ${ANTLR_JAR_LOCATION} -Werror -Dlanguage=Cpp -lib ${PROJECT_SOURCE_DIR}/src/grammar/ -listener -visitor -o ${PROJECT_SOURCE_DIR}/src/generated/ -package hlasm_plugin::parser_library::generated hlasmparser.g4
   WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/src/grammar/"
   DEPENDS antlr4jar ${PROJECT_SOURCE_DIR}/src/grammar/hlasmparser.g4 ${PROJECT_SOURCE_DIR}/src/grammar/lex.tokens
   )

set(LIB_NOT_EXPORTED_SRC    
    ${PROJECT_SOURCE_DIR}/src/context/common_types.cpp
    ${PROJECT_SOURCE_DIR}/src/context/hlasm_context.cpp
    ${PROJECT_SOURCE_DIR}/src/context/macro.cpp
    ${PROJECT_SOURCE_DIR}/src/context/macro_param_data.cpp
    ${PROJECT_SOURCE_DIR}/src/context/variable.cpp
    ${PROJECT_SOURCE_DIR}/src/context/instruction.cpp
	${PROJECT_SOURCE_DIR}/src/parser_tools.cpp
    ${PROJECT_SOURCE_DIR}/src/ebcdic_encoding.cpp
    ${PROJECT_SOURCE_DIR}/src/semantics/keyword_expression.cpp
    ${PROJECT_SOURCE_DIR}/src/semantics/arithmetic_expression.cpp
    ${PROJECT_SOURCE_DIR}/src/semantics/logic_expression.cpp
    ${PROJECT_SOURCE_DIR}/src/semantics/character_expression.cpp
    ${PROJECT_SOURCE_DIR}/src/semantics/expression.cpp
    ${PROJECT_SOURCE_DIR}/src/semantics/concatenation.cpp
    ${PROJECT_SOURCE_DIR}/src/semantics/semantic_objects.cpp
    ${PROJECT_SOURCE_DIR}/src/semantics/semantic_analyzer.cpp
    ${PROJECT_SOURCE_DIR}/src/semantics/statement_processor.cpp
    ${PROJECT_SOURCE_DIR}/src/semantics/operand.cpp
    ${PROJECT_SOURCE_DIR}/src/semantics/expression_visitor.cpp
    ${PROJECT_SOURCE_DIR}/src/semantics/semantic_info.cpp
    ${PROJECT_SOURCE_DIR}/src/workspace.cpp
    ${PROJECT_SOURCE_DIR}/src/file_manager_impl.cpp
    ${PROJECT_SOURCE_DIR}/src/file_impl.cpp
    ${PROJECT_SOURCE_DIR}/src/library.cpp
	${PROJECT_SOURCE_DIR}/src/processor_file_impl.cpp
	${PROJECT_SOURCE_DIR}/src/parser_error_listener.cpp
	${PROJECT_SOURCE_DIR}/src/parser_impl.cpp
	${PROJECT_SOURCE_DIR}/src/diagnostic.cpp
	${PROJECT_SOURCE_DIR}/src/semantics/semantic_info.cpp
	${PROJECT_SOURCE_DIR}/src/checking/instruction_checker.cpp
	${PROJECT_SOURCE_DIR}/src/checking/assembler_instruction.cpp

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
add_library(parser_library SHARED
    ${LIB_NOT_EXPORTED_SRC}
    ${GENERATED_SRC}
)

generate_export_header(parser_library
			EXPORT_FILE_NAME ${PROJECT_SOURCE_DIR}/src/generated/export/parser_library_export.h)

target_link_libraries(parser_library antlr4-runtime)
if(CMAKE_COMPILER_IS_GNUCXX)
	target_link_libraries(parser_library stdc++fs)
endif()

target_include_directories(parser_library
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/include
		${PROJECT_SOURCE_DIR}/src/generated/
		${PROJECT_SOURCE_DIR}/src/generated/export
)

ADD_CUSTOM_COMMAND (TARGET parser_library POST_BUILD      # Adds a post-build event to MyTest
    COMMAND ${CMAKE_COMMAND} -E copy_directory  # which executes "cmake - E copy_if_different..."
        "${ANTLR4CPP_LIBS}"      # <--this is in-file
        "${PROJECT_BINARY_DIR}/../bin/" )  

add_dependencies(parser_library antlr4jar GenerateParser)
add_dependencies(parser_library jsonrpcppext)



if(BUILD_TESTING)
	add_executable(library_test test/test.cpp
    ${LIB_NOT_EXPORTED_SRC}
	${GENERATED_SRC}
	)

	target_include_directories(library_test
    PUBLIC 
        "${PROJECT_SOURCE_DIR}/include"
	PUBLIC 
        "${PROJECT_SOURCE_DIR}/test/export"
	)

	ADD_CUSTOM_COMMAND (TARGET library_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory  
        "${ANTLR4CPP_LIBS}"      
        "${PROJECT_BINARY_DIR}/../bin/" )  

	add_custom_target(library_tests_copy
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${PROJECT_SOURCE_DIR}/test/res ${CMAKE_BINARY_DIR}/bin/test/library)

	target_link_libraries(library_test gmock)
	target_link_libraries(library_test antlr4-runtime)
	if(CMAKE_COMPILER_IS_GNUCXX)
		target_link_libraries(library_test stdc++fs)
    endif()
    
	add_dependencies(library_test library_tests_copy)
	add_dependencies(library_test antlr4jar)
	add_dependencies(library_test jsonrpcppext)
    gtest_discover_tests(library_test)
endif()
