cmake_minimum_required (VERSION 3.5)

PROJECT(HlasmLanguageServer)

# compile sources to the executable
add_executable(HlasmLanguageServer 
	${PROJECT_SOURCE_DIR}/src/main.cpp
	${PROJECT_SOURCE_DIR}/src/logger.cpp
	${PROJECT_SOURCE_DIR}/src/lsp_dispatcher.cpp
	${PROJECT_SOURCE_DIR}/src/server.cpp
	)

# link executable with libraries

add_dependencies(HlasmLanguageServer jsonrpcppext)

target_link_libraries(HlasmLanguageServer  jsonrpcpp)
target_link_libraries(HlasmLanguageServer  HlasmParserLibrary)
