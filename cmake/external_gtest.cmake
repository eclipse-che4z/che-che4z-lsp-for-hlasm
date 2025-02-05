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

project(googletest-download NONE)

include(FetchContent)

set(INSTALL_GTEST Off)

message("Populating googletest")
FetchContent_Declare(googletest
    GIT_REPOSITORY      https://github.com/google/googletest.git
    GIT_TAG             v1.15.2
    LOG_DOWNLOAD        ON
    GIT_PROGRESS        1
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(googletest)
target_compile_features(gmock PUBLIC cxx_std_20)
target_compile_features(gmock_main PUBLIC cxx_std_20)
target_compile_features(gtest PUBLIC cxx_std_20)
target_compile_features(gtest_main PUBLIC cxx_std_20)

set_target_properties(gmock PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(gmock_main PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(gtest PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(gtest_main PROPERTIES CXX_EXTENSIONS OFF)
