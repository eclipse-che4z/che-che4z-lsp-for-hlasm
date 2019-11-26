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
PROJECT(JSON_fetcher)
INCLUDE(ExternalProject)
FIND_PACKAGE(Git REQUIRED)

############ Download and Generate runtime #################
set(JSON_EXTERNAL_ROOT ${CMAKE_BINARY_DIR}/externals/json)

# external repository
set(JSON_EXTERNAL_REPO "https://github.com/nlohmann/json.git")
set(JSON_EXTERNAL_TAG  "v3.3.0")


# download runtime environment
ExternalProject_ADD(
  json
  PREFIX             ${JSON_EXTERNAL_ROOT}
  GIT_REPOSITORY     ${JSON_EXTERNAL_REPO}
  GIT_TAG            ${JSON_EXTERNAL_TAG}
  GIT_SHALLOW        ON
  TIMEOUT            10
  LOG_DOWNLOAD       ON
  GIT_PROGRESS       1
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
  TEST_COMMAND      ""
)

ExternalProject_Get_Property(json INSTALL_DIR)

set(JSON_INCLUDE_DIRS ${INSTALL_DIR}/src/json/single_include/nlohmann)


