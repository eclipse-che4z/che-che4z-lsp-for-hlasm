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

cmake_minimum_required(VERSION 3.23)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)

PROJECT(HlasmServer)

SET (CMAKE_CXX_STANDARD 20)
SET (CMAKE_CXX_STANDARD_REQUIRED ON)
SET (CMAKE_CXX_EXTENSIONS OFF)

# Global settings
set(GLOBAL_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR}/install)

# Sets global output directory for single configuration (GCC)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${GLOBAL_OUTPUT_PATH})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${GLOBAL_OUTPUT_PATH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${GLOBAL_OUTPUT_PATH})

# Sets global output directory for sub-configurations (msvc, mingw)
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${GLOBAL_OUTPUT_PATH})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${GLOBAL_OUTPUT_PATH})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${GLOBAL_OUTPUT_PATH})
endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)


# Detect build type, fallback to release and throw a warning if use didn't specify any
if(NOT CMAKE_BUILD_TYPE)
  message(WARNING "Build type not set, falling back to Release mode.
 To specify build type use:
 -DCMAKE_BUILD_TYPE=<mode> where <mode> is Debug or Release.")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
       "Choose the type of build, options are: Debug Release."
       FORCE)
endif(NOT CMAKE_BUILD_TYPE)

#the option is here, but we are not able to build with clang and libstdc++
option(WITH_LIBCXX "Building with clang++ and libc++(in Linux). To enable with: -DWITH_LIBCXX=On" On)
option(WITH_STATIC_CRT "(Visual C++) Enable to statically link CRT, which avoids requiring users to install the redistribution package.
 To disable with: -DWITH_STATIC_CRT=Off" On)
option(DISCOVER_TESTS "Enables CTest by executing tests and discovering test cases. To disable with: -DDISCOVER_TESTS=Off" On)
option(BUILD_VSIX "When disabled, the VS Code client is not built and it is not packaged into vsix." On)
option(BUILD_FUZZER "Enable building of the fuzzer. Tested with clang and libstdc++ (enable with -DWITH_LIBCXX=Off)" Off)
option(USE_STATIC_ANTLR "Always use static ANTLR library." On)

set(USE_PRE_GENERATED_GRAMMAR "" CACHE PATH "Directory with pre-generated grammar files")
set(LANGUAGE_SERVER_BIN_SUBDIR "" CACHE STRING "Subdirectory for the language server binary in the client project")

if(BUILD_SHARED_LIBS AND WITH_STATIC_CRT AND MSVC)
  message(WARNING "Building shared libraries with static CRT!")
endif()

if(BUILD_FUZZER AND WITH_LIBCXX)
  message(WARNING "Building fuzzer with libc++, use of libstdc++ recommended.")
endif()

if(BUILD_FUZZER AND BUILD_SHARED_LIBS)
  message(ERROR "Building fuzzer with dynamic libraries not supported.")
endif()

if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(WITH_LIBCXX Off)
endif()

LIST( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake )

include(compiler_flags)

if(BUILD_SHARED_LIBS)
  SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) 
  SET(CMAKE_SKIP_BUILD_RPATH FALSE)
  if(APPLE)
    set(CMAKE_MACOSX_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH "@executable_path")
  else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
  endif()
endif()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

find_package(Filesystem REQUIRED)

include(CTest)

include(external_antlr4cpp)

include(external_json)

include(external_uri)

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
    ${ANTLR4_RUNTIME}
    network-uri
    FILE_SET public_headers
)
