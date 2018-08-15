cmake_minimum_required (VERSION 3.10)

PROJECT(language_server)

include(GoogleTest)

set(SOURCES ${PROJECT_SOURCE_DIR}/src/logger.cpp
	${PROJECT_SOURCE_DIR}/src/lsp_dispatcher.cpp
	${PROJECT_SOURCE_DIR}/src/server.cpp)

# compile sources to the executable
add_executable(language_server 
	${PROJECT_SOURCE_DIR}/src/main.cpp
	${SOURCES}
	)

# link executable with libraries

add_dependencies(language_server jsonrpcppext)

target_link_libraries(language_server jsonrpcpp)
target_link_libraries(language_server parser_library)



if(BUILD_TESTING)
	add_executable(server_test test/test.cpp
		${SOURCES}
	)
	add_dependencies(server_test jsonrpcppext)
	target_link_libraries(server_test gmock_main)
	target_link_libraries(server_test jsonrpcpp)
	gtest_discover_tests(server_test)
endif()
