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

project(boost-asio)

include(FetchContent)

FetchContent_Declare(
  boost_ext
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG        asio-1-12-2
  GIT_SHALLOW    ON
  LOG_DOWNLOAD   ON
  GIT_PROGRESS   1
)

if(NOT boost_ext_POPULATED)
    FetchContent_Populate(boost_ext)
endif()

add_library(boost-asio INTERFACE)
target_include_directories(boost-asio INTERFACE ${boost_ext_SOURCE_DIR}/asio/include)
