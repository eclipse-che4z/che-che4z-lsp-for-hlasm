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

add_executable(library_test)

target_sources(library_test PRIVATE
    aread_time_test.cpp
    async_macro_parsing.cpp
    common_testing.cpp
    common_testing.h
    diagnosable_ctx_test.cpp
    diagnostics_check_test.cpp
    diagnostics_sysvar_test.cpp
    gtest_stringers.cpp
    gtest_stringers.h
    message_consumer_mock.h
    metrics_test.cpp
    mock_parse_lib_provider.h
    output_handler_mock.h
    stability_test.cpp
    utf_conv_test.cpp
    workspace_manager_response_mock.h
    workspace_manager_test.cpp
)

add_subdirectory(checking)
add_subdirectory(config)
add_subdirectory(context)
add_subdirectory(debugging)
add_subdirectory(expressions)
add_subdirectory(lexing)
add_subdirectory(lsp)
add_subdirectory(parsing)
add_subdirectory(processing)
add_subdirectory(semantics)
add_subdirectory(workspace)

if(MSVC)
    target_compile_options(library_test PRIVATE /bigobj)
endif()

add_custom_target(library_tests_copy
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${PROJECT_SOURCE_DIR}/test/res ${CMAKE_BINARY_DIR}/bin/test/library)

target_include_directories(library_test
    PRIVATE
    ../src
    ${GENERATED_GRAMMER_FOLDER}
    ${GENERATED_FOLDER}
)
target_link_libraries(library_test PRIVATE parser_library)
target_link_libraries(library_test PRIVATE gmock_main)
if (BUILD_SHARED_LIBS)
    set_target_properties(library_test PROPERTIES COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
endif()
target_link_libraries(library_test PRIVATE ${ANTLR4_RUNTIME})
target_link_libraries(library_test PRIVATE std::filesystem)
target_link_libraries(library_test PRIVATE nlohmann_json::nlohmann_json)

add_dependencies(library_test library_tests_copy)

if(DISCOVER_TESTS)
    gtest_discover_tests(library_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin DISCOVERY_TIMEOUT 120)
endif()
