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

set(CHECK_CI "${GLOBAL_OUTPUT_PATH}/npm_ci.stamp")
set(CHECK_BUILD "${GLOBAL_OUTPUT_PATH}/npm_build.stamp")

if (WIN32)
	set(CALL "call")
else()
	set(CALL "")
endif()

if(WIN32)
	set(ANTLR_VERSION_SUFFIX "")
else()
	set(ANTLR_VERSION_SUFFIX .${ANTLR4CPP_EXTERNAL_TAG})
endif()

set(BIN_EXTRAS "")
if(EMSCRIPTEN)
	set(BIN_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/bin/wasm/")
	list(APPEND BIN_EXTRAS "$<TARGET_FILE_DIR:language_server>/language_server.js" "$<TARGET_FILE_DIR:language_server>/language_server.worker.js" "$<TARGET_FILE_DIR:language_server>/language_server.wasm")
elseif(WIN32)
	set(BIN_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/bin/win32/")
elseif(APPLE)
	set(BIN_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/bin/darwin/")
else()
	set(BIN_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/bin/linux/")
endif()


set(COPY_PARSER_LIBRARY_COMMAND "")
set(COPY_ANTLR_LIBRARY_COMMAND "")
if(BUILD_SHARED_LIBS)
	set(COPY_PARSER_LIBRARY_COMMAND
		${CMAKE_COMMAND} -E copy $<TARGET_FILE:parser_library> ${BIN_FOLDER})

	if(APPLE)
		set(COPY_ANTLR_LIBRARY_COMMAND
			${CMAKE_COMMAND} -E copy ${GLOBAL_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}antlr4-runtime${ANTLR_VERSION_SUFFIX}${CMAKE_SHARED_LIBRARY_SUFFIX} ${BIN_FOLDER})
	else()
		set(COPY_ANTLR_LIBRARY_COMMAND
			${CMAKE_COMMAND} -E copy ${GLOBAL_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}antlr4-runtime${CMAKE_SHARED_LIBRARY_SUFFIX}${ANTLR_VERSION_SUFFIX} ${BIN_FOLDER})
	endif()
endif()

#Check whether npm is installed and in path
find_program(NPM_RETURN npm)
if(NPM_RETURN MATCHES "NPM_RETURN-NOTFOUND")
    message(FATAL_ERROR "Cannot find npm. Are you sure npm is installed and in the path?" )
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
			${CALL} ${NPM_RETURN} --unsafe-perm run compile
			#the --unsafe-perm is needed when user with root rights runs the build on linux
		COMMAND
			${CMAKE_COMMAND} -E make_directory ${BIN_FOLDER}
		COMMAND
			${COPY_PARSER_LIBRARY_COMMAND}
		COMMAND
			${COPY_ANTLR_LIBRARY_COMMAND}
		COMMAND
			${CMAKE_COMMAND} -E copy  $<TARGET_FILE:language_server> ${BIN_EXTRAS} ${BIN_FOLDER}
		COMMAND
			npx vsce package --no-dependencies -o ${GLOBAL_OUTPUT_PATH}/vscode-hlasmplugin.vsix
		COMMAND 
			${CMAKE_COMMAND} -E touch ${CHECK_BUILD}
		DEPENDS
			${CHECK_CI} ${CMAKE_CURRENT_SOURCE_DIR} parser_library language_server
		WORKING_DIRECTORY
			${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(vsix
		ALL
		DEPENDS
			${CHECK_BUILD}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_dependencies(vsix language_server parser_library)
