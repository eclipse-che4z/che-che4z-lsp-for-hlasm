cmake_minimum_required (VERSION 3.5)

PROJECT(HlasmLanguageServer)

# compile sources to the executable
add_executable(HlasmLanguageServer 
	${PROJECT_SOURCE_DIR}/src/main.cpp)

# link executable with libraries
target_link_libraries(HlasmLanguageServer  antlr4-runtime)
target_link_libraries(HlasmLanguageServer  HlasmParserLibrary)