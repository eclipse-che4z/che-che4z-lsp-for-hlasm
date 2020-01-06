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


project(googletest-download NONE)

set(GTEST_EXTERNAL_ROOT ${CMAKE_BINARY_DIR}/externals/googletest)

include(ExternalProject)
ExternalProject_Add(googletest
  GIT_REPOSITORY    https://github.com/google/googletest.git
  GIT_TAG           release-1.10.0
  SOURCE_DIR        ${CMAKE_BINARY_DIR}/googletest-src
  BINARY_DIR        ${CMAKE_BINARY_DIR}/googletest-build
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
  TEST_COMMAND      ""
)
