# Copyright (c) 2022 Broadcom.
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

add_executable(hlasm_utils_test)

target_sources(hlasm_utils_test PRIVATE
	bk_tree_test.cpp
	encoding_test.cpp
	levenshtein_distance_test.cpp
	path_conversions_test.cpp
	platform_test.cpp
	resource_location_test.cpp
	unicode_text_test.cpp
	time_test.cpp
	task_test.cpp
)

target_link_libraries(hlasm_utils_test hlasm_utils)
target_link_libraries(hlasm_utils_test gmock_main)

if(DISCOVER_TESTS)
	gtest_discover_tests(hlasm_utils_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin DISCOVERY_TIMEOUT 120)	
endif()
