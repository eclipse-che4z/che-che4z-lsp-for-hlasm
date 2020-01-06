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

CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(uri_fetcher CXX)
INCLUDE(ExternalProject)
FIND_PACKAGE(Git REQUIRED)

if(WITH_LIBCXX)
  set(DISABLE_LIBCXX Off)
else()
  set(DISABLE_LIBCXX On)
endif()

# download runtime environment
ExternalProject_ADD(
  uri_ext
  PREFIX             ${CMAKE_BINARY_DIR}/externals/uri
  GIT_REPOSITORY     "https://github.com/cpp-netlib/uri.git"
  GIT_TAG            v1.1.0
  GIT_SHALLOW        ON
  TIMEOUT            10
  LOG_DOWNLOAD       ON
  GIT_PROGRESS       1
  CMAKE_ARGS -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} -DUri_DISABLE_LIBCXX=${DISABLE_LIBCXX} -DUri_BUILD_DOCS=OFF -DUri_BUILD_TESTS=OFF -DUri_USE_STATIC_CRT=${WITH_STATIC_CRT} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DUri_WARNINGS_AS_ERRORS=OFF
)

ExternalProject_Get_Property(uri_ext INSTALL_DIR)



set(URI_INCLUDE_DIRS ${INSTALL_DIR}/include/)

set(URI_LIBS "${INSTALL_DIR}/lib")


