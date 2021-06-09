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

FetchContent_Declare(googletest
    GIT_REPOSITORY      https://github.com/google/googletest.git
    GIT_TAG             release-1.10.0
    LOG_DOWNLOAD        ON
    GIT_PROGRESS        1
    )

FetchContent_GetProperties(googletest)

function(add_googletest)
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE BOOL "")
    set(BUILD_SHARED_LIBS Off)
    set(gtest_force_shared_crt ${WITH_STATIC_CRT} CACHE BOOL "" FORCE)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR} EXCLUDE_FROM_ALL)
    unset(CMAKE_SUPPRESS_DEVELOPER_WARNINGS)
endfunction()

if(NOT googletest_POPULATED)
    message("Populating GTest")
    FetchContent_Populate(googletest)
    add_googletest()
endif()
