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

PROJECT(HlasmServer)

SET (CMAKE_CXX_STANDARD 17)
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
option(FS_PATH "Provide path to libc++fs for installation on MacOS" "")
option(BUILD_VSIX "When disabled, the VS Code client is not built and it is not packaged into vsix." On)

if(BUILD_SHARED_LIBS AND WITH_STATIC_CRT AND MSVC)
  message(WARNING "Building shared libraries with static CRT!")
endif()

if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(WITH_LIBCXX Off)
endif()


if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(UUID REQUIRED uuid)
endif()
if(APPLE)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)
endif()

if (MSVC_VERSION)
  string(REGEX REPLACE " /W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /wd4251 /wd4996")
  set(MY_CXX_WARNING_FLAGS "  /w35038")
else()
  set(MY_CXX_WARNING_FLAGS "  -Wall -Wextra -Wno-attributes -Wno-unknown-pragmas")
endif()

set(CMAKE_CXX_FLAGS                  "${CMAKE_CXX_FLAGS} ${MY_CXX_WARNING_FLAGS}")
string(REGEX REPLACE " /Ob[0-4]" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
string(REGEX REPLACE " /Ob[0-4]" "" CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")
string(REGEX REPLACE " /Ob[0-4]" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
string(REGEX REPLACE " /Ob[0-4]" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

if (MSVC_VERSION)
  if(WITH_STATIC_CRT)
    set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_RELEASE
        )
    foreach(CompilerFlag ${CompilerFlags})
      string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")

      string(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    endforeach()
  endif()

  string(REPLACE "/ZI" "/Zi" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
  string(REPLACE "/ZI" "/Zi" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
  
  set(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG} /Od /MP ${MY_CXX_WARNING_FLAGS}")
  set(CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS_MINSIZEREL} /O1 /Oi /Ob2 /Gy /MP /DNDEBUG ${MY_CXX_WARNING_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Oi /Ob2 /Gy /MP /DNDEBUG ${MY_CXX_WARNING_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /O2 /Oi /Ob2 /Gy /MP ${MY_CXX_WARNING_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} ")
else()
  set(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g ${MY_CXX_WARNING_FLAGS}")
  set(CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os -DNDEBUG ${MY_CXX_WARNING_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG ${MY_CXX_WARNING_FLGAS}")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2 -g ${MY_CXX_WARNING_FLAGS}")
endif()

if(WITH_LIBCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

if(APPLE)
  if(WITH_STATIC_CRT)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc++abi ${LLVM_PATH}/lib/libc++.a")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lc++abi ${LLVM_PATH}/lib/libc++.a")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nostdlib++")
  endif()
endif()

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

set(CMAKE_FLAGS_PASS -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} -DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS} -DCMAKE_SHARED_LINKER_FLAGS=${CMAKE_SHARED_LINKER_FLAGS})
#indicates whether we have to link filesystem library explicitly
if(UNIX)
  try_run(TRY_LINK_RUN_RESULT TRY_LINK_COMPILE_RESULT ${CMAKE_BINARY_DIR}/try_filesystem_link ${CMAKE_SOURCE_DIR}/cmake/try_filesystem_link.cpp
  CMAKE_FLAGS ${CMAKE_FLAGS_PASS}
  )

  #if the program ended with non-zero or a compile error occured
  if(NOT (TRY_LINK_RUN_RESULT EQUAL "0") OR NOT ${TRY_LINK_COMPILE_RESULT})
    set(FILESYSTEM_LINK On)
    if(APPLE)
      set(FILESYSTEM_LIBRARY "${LLVM_PATH}/lib/libc++fs.a")
    elseif(WITH_LIBCXX)
      set(FILESYSTEM_LIBRARY c++fs)
    else()
      set(FILESYSTEM_LIBRARY stdc++fs)
    endif()
  endif()

  #test filesystem setup
  set(TEST_LINK_LIBS "")
  if(FILESYSTEM_LINK)
    set(TEST_LINK_LIBS "-DLINK_LIBRARIES=${FILESYSTEM_LIBRARY}")
  endif()
  
  try_run(TEST_LINK_RUN_RESULT TEST_LINK_COMPILE_RESULT ${CMAKE_BINARY_DIR}/try_filesystem_link_test ${CMAKE_SOURCE_DIR}/cmake/try_filesystem_link.cpp
    CMAKE_FLAGS ${CMAKE_FLAGS_PASS} ${TEST_LINK_LIBS}
    )

  if(NOT (TEST_LINK_RUN_RESULT EQUAL "0") OR NOT ${TEST_LINK_COMPILE_RESULT})
    message(FATAL_ERROR "Filesystem link error: ${CMAKE_SOURCE_DIR}/cmake/try_filesystem_link.cpp could not be compiled.")
  endif()
endif()

include(CTest)

LIST( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake )
  include( external_antlr4cpp )
  include_directories( ${ANTLR4CPP_INCLUDE_DIRS} )
  link_directories( ${ANTLR4CPP_LIBS} )

include( external_json )
include_directories( ${JSON_INCLUDE_DIRS} )

include(external_uri)
include_directories( ${URI_INCLUDE_DIRS} )
link_directories( ${URI_LIBS} )

include(external_boost)
include_directories( ${BOOST_INCLUDE_DIRS} )


#Testing setup
if(BUILD_TESTING)
  set(GTEST_SHARED_CRT NOT ${WITH_STATIC_CRT})
  # Download and unpack googletest at configure time
  configure_file(cmake/external_gtest.cmake googletest-download/CMakeLists.txt)
  execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
  if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
  endif()
  execute_process(COMMAND ${CMAKE_COMMAND} --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
  if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
  endif()

  # Prevent overriding the parent project's compiler/linker
  # settings on Windows
  set(gtest_force_shared_crt ${GTEST_SHARED_CRT} CACHE BOOL "" FORCE)

  #we want to link google test staticly under all circumstances
  set(BUILD_SHARED_LIBS_TMP ${BUILD_SHARED_LIBS})
  set(BUILD_SHARED_LIBS Off)
  # Add googletest directly to our build. This defines
  # the gtest and gtest_main targets.
  add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
           ${CMAKE_BINARY_DIR}/googletest-build
           EXCLUDE_FROM_ALL)
  set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_TMP})
endif()

# Global link directories
link_directories(${GLOBAL_OUTPUT_PATH} ${ANTLR4CPP_LIBS})

# Libraries (+ their includes)
add_subdirectory(parser_library)
include_directories(parser_library/include)


# Applications
add_subdirectory(language_server)
add_subdirectory(dummy)
add_subdirectory(benchmark)

if(BUILD_VSIX)
  add_subdirectory(clients)
endif()



# Installation

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(EXECUTABLE "")
  set(LIBRARY ".so")
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(EXECUTABLE ".exe")
  set(LIBRARY ".dll")
endif()

# Copies files from build directory into install directory.
file(GLOB_RECURSE executables "${GLOBAL_OUTPUT_PATH}/*${EXECUTABLE}" )
install(FILES ${executables} DESTINATION "bin")


file(GLOB_RECURSE libraries "${GLOBAL_OUTPUT_PATH}/*${LIBRARY}" "${ANTLR4CPP_LIBS}/*${LIBRARY}")
install(FILES ${libraries} DESTINATION "bin")


# SDK Installation
install(DIRECTORY "${PROJECT_SOURCE_DIR}/parser_library/include/" DESTINATION "include" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${PROJECT_SOURCE_DIR}/parser_library/include/" DESTINATION "include" FILES_MATCHING PATTERN "*.hpp")
