cmake_minimum_required (VERSION 3.10)

PROJECT(private_test)

include(GoogleTest)

if(BUILD_TESTING)
	add_executable(private_test test/test.cpp
		${HLASMSERVER_SOURCES}
	)
	
	target_link_libraries(private_test gmock_main)
	target_link_libraries(private_test parser_library)

endif()
