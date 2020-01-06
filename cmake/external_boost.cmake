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

project(boost-asio)

include(ExternalProject)

FIND_PACKAGE(Git REQUIRED)
ExternalProject_Add(boost_ext
  PREFIX            ${CMAKE_BINARY_DIR}/externals/boost
  GIT_REPOSITORY    https://github.com/chriskohlhoff/asio.git
  GIT_TAG           asio-1-12-1
  GIT_SHALLOW       ON
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
  TEST_COMMAND      ""
)


ExternalProject_Get_Property(boost_ext INSTALL_DIR)
set(BOOST_INCLUDE_DIRS ${INSTALL_DIR}/src/boost_ext/asio/include/)