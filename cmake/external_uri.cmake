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

PROJECT(uri_fetcher CXX)
INCLUDE(ExternalProject)
FIND_PACKAGE(Git REQUIRED)

include(FetchContent)

# download runtime environment
FetchContent_Declare(
  uri_ext
  GIT_REPOSITORY https://github.com/cpp-netlib/uri.git
  GIT_TAG        9f477677a1fefa235027a64c47db5fca53e7f61d
  GIT_SHALLOW    ON
  LOG_DOWNLOAD   ON
  GIT_PROGRESS   1
)

FetchContent_GetProperties(uri_ext)

function(add_uri_ext)
    if(WITH_LIBCXX)
      set(Uri_DISABLE_LIBCXX Off)
    else()
      set(Uri_DISABLE_LIBCXX On)
    endif()
    set(Uri_BUILD_TESTS Off)
    set(Uri_BUILD_DOCS Off)
    set(Uri_WARNINGS_AS_ERRORS Off)

    add_subdirectory(${uri_ext_SOURCE_DIR}/src ${uri_ext_BINARY_DIR} EXCLUDE_FROM_ALL)

    target_include_directories(network-uri PRIVATE ${uri_ext_SOURCE_DIR}/src)
    target_include_directories(network-uri PUBLIC ${uri_ext_SOURCE_DIR}/include)
endfunction()

if(NOT uri_ext_POPULATED)
    message("Populating netlib uri")
    FetchContent_Populate(uri_ext)
    add_uri_ext()
endif()
