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

CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(antlr4cpp_fetcher CXX)
INCLUDE(ExternalProject)
FIND_PACKAGE(Git REQUIRED)

# only JRE required
FIND_PACKAGE(Java COMPONENTS Runtime REQUIRED)

############ Download and Generate runtime #################
set(ANTLR4CPP_EXTERNAL_ROOT ${CMAKE_BINARY_DIR}/externals/antlr4cpp)

# external repository
# GIT_REPOSITORY     https://github.com/antlr/antlr4.git
set(ANTLR4CPP_EXTERNAL_REPO "https://github.com/antlr/antlr4.git")
set(ANTLR4CPP_EXTERNAL_TAG  "4.7.2")

set(ANTLR_VERSION "4.7.2")

if(BUILD_SHARED_LIBS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        set(ANTLR4_LIB_FILENAME "antlr4-runtime.dll")
    elseif(APPLE)
        set(ANTLR4_LIB_FILENAME "antlr4-runtime.${ANTLR_VERSION}.dylib")
    else()
        set(ANTLR4_LIB_FILENAME "antlr4-runtime.so.${ANTLR_VERSION}")
    endif()
else()
	if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        set(ANTLR4_LIB_FILENAME "antlr4-runtime-static.lib")
    else()
        set(ANTLR4_LIB_FILENAME "libantlr4-runtime.a")
	endif()
endif()

# download runtime environment
ExternalProject_ADD(
  antlr4cpp
  PREFIX             ${ANTLR4CPP_EXTERNAL_ROOT}
  GIT_REPOSITORY     ${ANTLR4CPP_EXTERNAL_REPO}
  GIT_TAG            ${ANTLR4CPP_EXTERNAL_TAG}
  GIT_SHALLOW        ON
  # the fix for https://github.com/antlr/antlr4/issues/2550
  PATCH_COMMAND      ${CMAKE_COMMAND} -DGIT_EXECUTABLE=${GIT_EXECUTABLE} -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR} -DCMAKE_BINARY_DIR=${CMAKE_BINARY_DIR} -P ${PROJECT_SOURCE_DIR}/cmake/apply_patch.cmake
  TIMEOUT            10
  LOG_DOWNLOAD       ON
  GIT_PROGRESS       1
  CMAKE_ARGS -G ${CMAKE_GENERATOR} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} -DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS} -DCMAKE_SHARED_LINKER_FLAGS=${CMAKE_SHARED_LINKER_FLAGS} -DWITH_LIBCXX=${WITH_LIBCXX} -DBUILD_SHARED_LIBS=ON -DWITH_STATIC_CRT=${WITH_STATIC_CRT} -DCMAKE_MACOSX_RPATH=ON -DBUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  SOURCE_SUBDIR runtime/Cpp
  LOG_CONFIGURE ON
  LOG_BUILD ON
  BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/externals/antlr4cpp/lib/${ANTLR4_LIB_FILENAME}
)

ExternalProject_Get_Property(antlr4cpp INSTALL_DIR)


# set ANTLR jar location
set(ANTLR_JAR_LOCATION 
	${ANTLR4CPP_EXTERNAL_ROOT}/src/antlr4cpp/tool/target/antlr4-4.7.2-complete.jar)

add_custom_command(
    OUTPUT
        ${ANTLR_JAR_LOCATION}
    DEPENDS
        antlr4cpp
    COMMAND
        mvn -DskipTests install
    COMMENT
        "Building ANTLR jar..."
    WORKING_DIRECTORY 
        ${ANTLR4CPP_EXTERNAL_ROOT}/src/antlr4cpp/tool/
    VERBATIM
    )

add_custom_target(antlr4jar
                   DEPENDS 
                      ${ANTLR_JAR_LOCATION})

list(APPEND ANTLR4CPP_INCLUDE_DIRS ${INSTALL_DIR}/include/antlr4-runtime)
foreach(src_path misc atn dfa tree support)
  list(APPEND ANTLR4CPP_INCLUDE_DIRS ${INSTALL_DIR}/include/antlr4-runtime/${src_path})
endforeach(src_path)

set(ANTLR4CPP_LIBS "${INSTALL_DIR}/lib")

if(BUILD_SHARED_LIBS)
    set(ANTLR4_RUNTIME ${ANTLR4_LIB_FILENAME})
else()
    set(ANTLR4_RUNTIME "${ANTLR4CPP_LIBS}/${ANTLR4_LIB_FILENAME}")
endif()
