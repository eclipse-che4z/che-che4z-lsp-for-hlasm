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

target_compile_features(server_test PRIVATE cxx_std_20)
target_compile_options(server_test PRIVATE ${HLASM_EXTRA_FLAGS})
set_target_properties(server_test PROPERTIES CXX_EXTENSIONS OFF)

target_sources(server_test PRIVATE
    blocking_queue_test.cpp
    external_file_reader_test.cpp
    json_channel_mock.h
    message_router_test.cpp
    regress_test.cpp
    response_provider_mock.h
    send_message_provider_mock.h
    server_options_test.cpp
    ws_mngr_mock.h
    ws_mngr_req_mock.h
    telemetry_test.cpp
    virtual_file_provider_test.cpp
)

if (NOT EMSCRIPTEN)
    target_sources(server_test PRIVATE
        channel_test.cpp
        stream_helper_test.cpp
    )
endif()

add_subdirectory(dap)
add_subdirectory(lsp)

target_include_directories(server_test PRIVATE ../src)

target_link_libraries(server_test PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(server_test PRIVATE boost-asio)
target_link_libraries(server_test PRIVATE hlasm_language_server_base)

target_link_libraries(server_test PRIVATE gmock_main)
if (BUILD_SHARED_LIBS)
    set_target_properties(server_test PROPERTIES COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
endif()
target_link_libraries(server_test PRIVATE parser_library)

target_link_options(server_test PRIVATE ${HLASM_EXTRA_LINKER_FLAGS})

if(DISCOVER_TESTS)
    gtest_discover_tests(server_test WORKING_DIRECTORY $<TARGET_FILE_DIR:server_test> DISCOVERY_TIMEOUT 120)
endif()
