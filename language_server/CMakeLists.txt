cmake_minimum_required (VERSION 3.10)

PROJECT(HlasmLanguageServer)

include(GoogleTest)

set(SOURCES ${PROJECT_SOURCE_DIR}/src/logger.cpp
	${PROJECT_SOURCE_DIR}/src/lsp_dispatcher.cpp
	${PROJECT_SOURCE_DIR}/src/server.cpp)

# compile sources to the executable
add_executable(HlasmLanguageServer 
	${PROJECT_SOURCE_DIR}/src/main.cpp
	${SOURCES}
	)

# link executable with libraries

add_dependencies(HlasmLanguageServer jsonrpcppext)

target_link_libraries(HlasmLanguageServer jsonrpcpp)
target_link_libraries(HlasmLanguageServer HlasmParserLibrary)



if(BUILD_TESTING)
	add_executable(serverTest test/test.cpp
		${SOURCES}
	)
	add_dependencies(serverTest jsonrpcppext)
	target_link_libraries(serverTest gmock_main)
	target_link_libraries(serverTest jsonrpcpp)
	gtest_discover_tests(serverTest)
endif()
