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

message("Populating ASIO")
FetchContent_Declare(
  boost_ext
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG        asio-1-21-0
  GIT_SHALLOW    ON
  LOG_DOWNLOAD   ON
  GIT_PROGRESS   1
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(boost_ext)

add_library(boost-asio INTERFACE)
target_include_directories(boost-asio INTERFACE ${boost_ext_SOURCE_DIR}/asio/include)
