# Copyright (c) 2021 Broadcom.
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

add_executable(server_test)
generate_emscripten_node_runner(server_test)

target_sources(server_test PRIVATE
	blocking_queue_test.cpp
	channel_test.cpp
	external_file_reader_test.cpp
	json_channel_mock.h
	message_router_test.cpp
	regress_test.cpp
	response_provider_mock.h
	send_message_provider_mock.h
	stream_helper_test.cpp
	ws_mngr_mock.h
	ws_mngr_req_mock.h
	telemetry_test.cpp
	virtual_file_provider_test.cpp
)

add_subdirectory(dap)
add_subdirectory(lsp)

target_link_libraries(server_test nlohmann_json::nlohmann_json)
target_link_libraries(server_test network-uri)
target_link_libraries(server_test boost-asio)
target_link_libraries(server_test language_server_base)

target_link_libraries(server_test gmock_main)
target_link_libraries(server_test network-uri)
target_link_libraries(server_test parser_library)

if(DISCOVER_TESTS)
	gtest_discover_tests(server_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin DISCOVERY_TIMEOUT 120)	
endif()
