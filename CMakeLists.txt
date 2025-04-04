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

cmake_minimum_required(VERSION 3.28)

PROJECT(HlasmLanguageServer LANGUAGES CXX)

option(APPLE_STATIC_CRT "Try linking libc++ statically on Apple." Off)
option(DISCOVER_TESTS "Enables CTest by executing tests and discovering test cases. To disable with: -DDISCOVER_TESTS=Off" On)
option(BUILD_VSIX "When disabled, the VS Code client is not built and it is not packaged into vsix." On)
option(BUILD_VSIX_WEB "Packages Web version of the extension." Off)
option(BUILD_FUZZER "Enable building of the fuzzer. Tested with clang and libstdc++." Off)

set(LANGUAGE_SERVER_BIN_SUBDIR "" CACHE STRING "Subdirectory for the language server binary in the client project")
option(HLASM_DEV_GUESS_BIN_SUBDIR "Try to guess the LANGUAGE_SERVER_BIN_SUBDIR value" Off)

if(BUILD_SHARED_LIBS AND WITH_STATIC_CRT AND MSVC)
  message(WARNING "Building shared libraries with static CRT!")
endif()

if(BUILD_FUZZER AND BUILD_SHARED_LIBS)
  message(ERROR "Building fuzzer with dynamic libraries not supported.")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(compiler_flags)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

include(CTest)

include(external_json)

include(external_boost)

#Testing setup
if(BUILD_TESTING)
    include(external_gtest)
endif()

# Libraries (+ their includes)
add_subdirectory(parser_library)

# Applications
add_subdirectory(language_server)
add_subdirectory(benchmark)

add_subdirectory(utils)

if(BUILD_VSIX)
  add_subdirectory(clients)
endif()

# Installation

install(TARGETS
    hlasm_language_server
    parser_library
    hlasm_utils
    FILE_SET public_headers
)
