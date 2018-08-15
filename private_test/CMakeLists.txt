cmake_minimum_required (VERSION 3.10)

PROJECT(HlasmPrivateTest)

include(GoogleTest)

if(BUILD_TESTING)
	add_executable(privateTest test/test.cpp
		${HLASMSERVER_SOURCES}
	)
	
	target_link_libraries(privateTest gmock_main)
	target_link_libraries(privateTest jsonrpcpp)
	target_link_libraries(privateTest antlr4-runtime)
	target_link_libraries(privateTest parser_library)
	gtest_discover_tests(privateTest)


endif()
