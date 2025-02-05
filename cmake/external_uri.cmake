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

message("Populating netlib uri")
# download runtime environment
FetchContent_Declare(
  uri_ext
  GIT_REPOSITORY https://github.com/cpp-netlib/uri.git
  GIT_TAG        9f477677a1fefa235027a64c47db5fca53e7f61d
  GIT_SHALLOW    ON
  LOG_DOWNLOAD   ON
  GIT_PROGRESS   1
  PATCH_COMMAND  ${CMAKE_COMMAND} -DGIT_EXECUTABLE=${GIT_EXECUTABLE} -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR} -DCMAKE_BINARY_DIR=${CMAKE_BINARY_DIR} -P ${PROJECT_SOURCE_DIR}/cmake/apply_uri_patch.cmake
  EXCLUDE_FROM_ALL
  SOURCE_SUBDIR  dummy
)
FetchContent_MakeAvailable(uri_ext)
add_subdirectory(${uri_ext_SOURCE_DIR}/src ${uri_ext_BINARY_DIR} EXCLUDE_FROM_ALL)
target_include_directories(network-uri PRIVATE ${uri_ext_SOURCE_DIR}/src)
target_include_directories(network-uri PUBLIC ${uri_ext_SOURCE_DIR}/include)

target_compile_features(network-uri PUBLIC cxx_std_20)
set_target_properties(network-uri PROPERTIES CXX_EXTENSIONS OFF)
