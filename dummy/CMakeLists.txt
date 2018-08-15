cmake_minimum_required (VERSION 3.10)

PROJECT(HlasmDummy)

# compile sources to the executable
add_executable(HlasmDummy 
	${PROJECT_SOURCE_DIR}/src/main.cpp
	)

# link executable with libraries
target_link_libraries(HlasmDummy  antlr4-runtime)
target_link_libraries(HlasmDummy  HlasmParserLibrary)
