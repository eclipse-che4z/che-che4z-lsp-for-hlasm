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

PROJECT(language_server)

include(GoogleTest)

set(SOURCES ${PROJECT_SOURCE_DIR}/src/logger.cpp
	${PROJECT_SOURCE_DIR}/src/feature.cpp
	${PROJECT_SOURCE_DIR}/src/server.cpp
	${PROJECT_SOURCE_DIR}/src/dispatcher.cpp
	${PROJECT_SOURCE_DIR}/src/request_manager.cpp
	${PROJECT_SOURCE_DIR}/src/lsp/lsp_server.cpp
	${PROJECT_SOURCE_DIR}/src/lsp/feature_workspace_folders.cpp
	${PROJECT_SOURCE_DIR}/src/lsp/feature_text_synchronization.cpp
	${PROJECT_SOURCE_DIR}/src/lsp/feature_language_features.cpp
	${PROJECT_SOURCE_DIR}/src/dap/dap_server.cpp
	${PROJECT_SOURCE_DIR}/src/dap/feature_launch.cpp
	${PROJECT_SOURCE_DIR}/src/dap/tcp_handler.cpp
	)

# compile sources to the executable
add_executable(language_server 
	${PROJECT_SOURCE_DIR}/src/main.cpp
	${SOURCES}
	)

if(NOT BUILD_SHARED_LIBS)
	set_target_properties(language_server PROPERTIES COMPILE_FLAGS "-DPARSER_LIBRARY_STATIC_DEFINE")
endif()

# link executable with libraries

add_dependencies(language_server json)
add_dependencies(language_server uri_ext)
add_dependencies(language_server boost_ext)

target_link_libraries(language_server network-uri)
target_link_libraries(language_server parser_library)
if(UNIX)
	target_link_libraries(language_server pthread)
endif()
if(BUILD_TESTING)
	add_executable(server_test test/test.cpp
		${SOURCES}
	)
	add_dependencies(server_test json)
	add_dependencies(server_test uri_ext)
	add_dependencies(server_test boost_ext)

	target_link_libraries(server_test gmock_main)
	target_link_libraries(server_test network-uri)
	target_link_libraries(server_test parser_library)

	if(DISCOVER_TESTS)
		gtest_discover_tests(server_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin DISCOVERY_TIMEOUT 30)	
	endif()
endif()
