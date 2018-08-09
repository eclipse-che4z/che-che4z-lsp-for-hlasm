cmake_minimum_required (VERSION 3.10)
INCLUDE(GenerateExportHeader)
include(GoogleTest)
project(HlasmParserLibrary)

SET (CMAKE_CXX_STANDARD 14)


# set ANTLR jar location
set(ANTLR_JAR_LOCATION 
	${ANTLR4CPP_EXTERNAL_ROOT}/src/antlr4cpp/tool/target/antlr4-4.7.1-complete.jar)

# generated grammar source files
set(GENERATED_SRC
   ${PROJECT_SOURCE_DIR}/src/generated/HlasmLexer.cpp
   ${PROJECT_SOURCE_DIR}/src/generated/HlasmParser.cpp
   ${PROJECT_SOURCE_DIR}/src/generated/HlasmParserBaseListener.cpp
   ${PROJECT_SOURCE_DIR}/src/generated/HlasmParserBaseVisitor.cpp
   ${PROJECT_SOURCE_DIR}/src/generated/HlasmParserListener.cpp
   ${PROJECT_SOURCE_DIR}/src/generated/HlasmParserVisitor.cpp
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
   "${Java_JAVA_EXECUTABLE}" -jar ${ANTLR_JAR_LOCATION} -Werror -Dlanguage=Cpp -listener -visitor -o ${PROJECT_SOURCE_DIR}/src/generated/ -package HlasmGenerated ${PROJECT_SOURCE_DIR}/src/grammar/HlasmLexer.g4 ${PROJECT_SOURCE_DIR}/src/grammar/HlasmParser.g4
   WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
   DEPENDS antlr4jar ${PROJECT_SOURCE_DIR}/src/grammar/HlasmLexer.g4 ${PROJECT_SOURCE_DIR}/src/grammar/HlasmParser.g4
   )

#Generate the shared library from the library sources
add_library(HlasmParserLibrary SHARED
    ${PROJECT_SOURCE_DIR}/src/HlasmParserLibrary.cpp
	${GENERATED_SRC}
)

generate_export_header(HlasmParserLibrary
			EXPORT_FILE_NAME ${PROJECT_SOURCE_DIR}/src/generated/HlasmParserLibrary_export.h)

target_link_libraries(HlasmParserLibrary  antlr4-runtime)

target_include_directories(HlasmParserLibrary
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/include
		${PROJECT_SOURCE_DIR}/src/generated/
)

ADD_CUSTOM_COMMAND (TARGET HlasmParserLibrary POST_BUILD      # Adds a post-build event to MyTest
    COMMAND ${CMAKE_COMMAND} -E copy_directory  # which executes "cmake - E copy_if_different..."
        "${ANTLR4CPP_LIBS}"      # <--this is in-file
        "${PROJECT_BINARY_DIR}/../bin/" )  

add_dependencies(HlasmParserLibrary antlr4jar )



if(BUILD_TESTING)
	add_executable(libraryTest test/test.cpp)
	target_link_libraries(libraryTest gmock_main)
	
	target_link_libraries(libraryTest HlasmParserLibrary)
	gtest_discover_tests(libraryTest)
endif()
