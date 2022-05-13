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

PROJECT(antlr4cpp_fetcher CXX)

INCLUDE(FetchContent)
FIND_PACKAGE(Git REQUIRED)

# only JRE required
FIND_PACKAGE(Java 11 COMPONENTS Runtime REQUIRED)

if(APPLE)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)
endif()

#Check whether maven is installed and in path
find_program(MVN_RETURN mvn)
if(MVN_RETURN MATCHES "MVN_RETURN-NOTFOUND")
    message(FATAL_ERROR "Cannot find mvn. Are you sure maven is installed and in the path?" )
endif()

# external repository
# GIT_REPOSITORY     https://github.com/antlr/antlr4.git
set(ANTLR4CPP_EXTERNAL_REPO "https://github.com/antlr/antlr4.git")
set(ANTLR4CPP_EXTERNAL_TAG  "4.10.1")
set(ANTLR_VERSION ${ANTLR4CPP_EXTERNAL_TAG})

FetchContent_Declare(
    antlr4cpp
    GIT_REPOSITORY https://github.com/antlr/antlr4.git
    GIT_TAG        ${ANTLR4CPP_EXTERNAL_TAG}
    GIT_SHALLOW    ON
    LOG_DOWNLOAD   ON
    GIT_PROGRESS   1
    PATCH_COMMAND  ${CMAKE_COMMAND} -DGIT_EXECUTABLE=${GIT_EXECUTABLE} -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR} -DCMAKE_BINARY_DIR=${CMAKE_BINARY_DIR} -P ${PROJECT_SOURCE_DIR}/cmake/apply_patch.cmake
)

FetchContent_GetProperties(antlr4cpp)

function(add_antlr4)
    set(PROJECT_SOURCE_DIR ${antlr4cpp_SOURCE_DIR}/runtime/Cpp)
    set(LIB_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)
    set(ANTLR_BUILD_CPP_TESTS Off)
    add_subdirectory(${antlr4cpp_SOURCE_DIR}/runtime/Cpp/runtime ${antlr4cpp_BINARY_DIR} EXCLUDE_FROM_ALL)

    target_include_directories(antlr4_shared INTERFACE ${antlr4cpp_SOURCE_DIR}/runtime/Cpp/runtime/src)
    target_include_directories(antlr4_static INTERFACE ${antlr4cpp_SOURCE_DIR}/runtime/Cpp/runtime/src)
    target_compile_definitions(antlr4_static INTERFACE ANTLR4CPP_STATIC=1)
endfunction()

if(NOT antlr4cpp_POPULATED)
    message("Populating antlr4")
    FetchContent_Populate(antlr4cpp)
    add_antlr4()

# set ANTLR jar location
    set(ANTLR_JAR_LOCATION
        ${antlr4cpp_SOURCE_DIR}/tool/target/antlr4-${ANTLR4CPP_EXTERNAL_TAG}-complete.jar)

    add_custom_command(
        OUTPUT
            ${ANTLR_JAR_LOCATION}
        COMMAND
            mvn -DskipTests install
        COMMENT
            "Building ANTLR jar..."
        WORKING_DIRECTORY
            ${antlr4cpp_SOURCE_DIR}/tool/
        VERBATIM
        )

    add_custom_target(antlr4jar
                       DEPENDS
                          ${ANTLR_JAR_LOCATION})
endif()

if(BUILD_SHARED_LIBS)
    set(ANTLR4_RUNTIME antlr4_shared)
else()
    set(ANTLR4_RUNTIME antlr4_static)
endif()
