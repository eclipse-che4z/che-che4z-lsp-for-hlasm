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

PROJECT(vscode-hlasmplugin)

set(CHECK_CI "${CMAKE_CURRENT_BINARY_DIR}/npm_ci.stamp")
set(CHECK_BUILD "${CMAKE_CURRENT_BINARY_DIR}/npm_build.stamp")

if (WIN32)
    set(CALL "call")
else()
    set(CALL "")
endif()

set(BIN_EXTRAS "")
set(BIN_FOLDER "")
if(NOT "${LANGUAGE_SERVER_BIN_SUBDIR}" STREQUAL "")
    set(BIN_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/bin/${LANGUAGE_SERVER_BIN_SUBDIR}/")
elseif(EMSCRIPTEN)
    set(BIN_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/bin/wasm/")
elseif(HLASM_DEV_GUESS_BIN_SUBDIR AND "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "AMD64")
        set(BIN_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/bin/win32_x64/")
    elseif ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "ARM64")
        set(BIN_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/bin/win32_arm64/")
    endif()
elseif(HLASM_DEV_GUESS_BIN_SUBDIR AND "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
        set(BIN_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/bin/linux_x64/")
    elseif ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64")
        set(BIN_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/bin/linux_arm64/")
    elseif ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64")
        set(BIN_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/bin/linux_arm64/")
    endif()
elseif(HLASM_DEV_GUESS_BIN_SUBDIR AND "${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
        set(BIN_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/bin/darwin_x64/")
    elseif ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64")
        set(BIN_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/bin/darwin_arm64/")
    endif()
endif()

if("${BIN_FOLDER}" STREQUAL "")
    message(FATAL_ERROR "LANGUAGE_SERVER_BIN_SUBDIR was not specified and could not have been guessed")
endif()

if(EMSCRIPTEN)
    list(APPEND BIN_EXTRAS "$<TARGET_FILE_DIR:hlasm_language_server>/hlasm_language_server.js" "$<TARGET_FILE_DIR:hlasm_language_server>/hlasm_language_server.wasm")
endif()


set(COPY_PARSER_LIBRARY_COMMAND "")
if(BUILD_SHARED_LIBS)
    set(COPY_PARSER_LIBRARY_COMMAND
        ${CMAKE_COMMAND} -E copy $<TARGET_FILE:parser_library> ${BIN_FOLDER})
endif()

#Check whether npm is installed and in path
find_program(NPM_RETURN npm)
if(NPM_RETURN MATCHES "NPM_RETURN-NOTFOUND")
    message(FATAL_ERROR "Cannot find npm. Are you sure npm is installed and in the path?" )
endif()

set(NPM_COMPILE_COMMAND "compile")
if(BUILD_VSIX_WEB)
    set(NPM_COMPILE_COMMAND "compile:web")
endif()

add_custom_command(
        OUTPUT
            ${CHECK_CI}
        COMMAND
            ${CALL} ${NPM_RETURN} --unsafe-perm ci
            #the --unsafe-perm is needed when user with root rights runs the build on linux
        COMMAND
            ${CMAKE_COMMAND} -E touch ${CHECK_CI}
        DEPENDS
            package.json package-lock.json build/bin/postinstall.js
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_command(
        OUTPUT
            ${CHECK_BUILD}
        COMMAND
            ${CALL} ${NPM_RETURN} --unsafe-perm run ${NPM_COMPILE_COMMAND}
            #the --unsafe-perm is needed when user with root rights runs the build on linux
        COMMAND
            ${CMAKE_COMMAND} -E make_directory ${BIN_FOLDER}
        COMMAND
            ${COPY_PARSER_LIBRARY_COMMAND}
        COMMAND
            ${CMAKE_COMMAND} -E copy  $<TARGET_FILE:hlasm_language_server> ${BIN_EXTRAS} ${BIN_FOLDER}
        COMMAND
            npx vsce package --no-dependencies -o ${CMAKE_CURRENT_BINARY_DIR}/vscode-hlasmplugin.vsix
        COMMAND
            ${CMAKE_COMMAND} -E touch ${CHECK_BUILD}
        DEPENDS
            ${CHECK_CI} ${CMAKE_CURRENT_SOURCE_DIR} parser_library hlasm_language_server
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(vsix
        ALL
        DEPENDS
            ${CHECK_BUILD}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_dependencies(vsix hlasm_language_server parser_library)
