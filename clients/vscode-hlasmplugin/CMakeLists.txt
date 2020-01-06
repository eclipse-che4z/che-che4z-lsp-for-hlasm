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

PROJECT(vscode-hlasmplugin)

set(CHECK "${GLOBAL_OUTPUT_PATH}/npm_install.stamp")

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

set(COPY_PARSER_LIBRARY_COMMAND "")
set(COPY_ANTLR_LIBRARY_COMMAND "")
if(BUILD_SHARED_LIBS)
	set(COPY_PARSER_LIBRARY_COMMAND
		${CMAKE_COMMAND} -E copy $<TARGET_FILE:parser_library> ${CMAKE_CURRENT_SOURCE_DIR}/bin/)
	set(COPY_ANTLR_LIBRARY_COMMAND
        ${CMAKE_COMMAND} -E copy ${GLOBAL_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}antlr4-runtime${CMAKE_SHARED_LIBRARY_SUFFIX}${ANTLR_VERSION_SUFFIX} ${CMAKE_CURRENT_SOURCE_DIR}/bin/)
	
endif()

add_custom_command(
		OUTPUT
			${CHECK}
		COMMAND
			${CALL} npm --unsafe-perm install
			#the --unsafe-perm is needed when user with root rights runs the build on linux
		COMMAND
			${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/bin/
		COMMAND
			${COPY_PARSER_LIBRARY_COMMAND}
		COMMAND
			${COPY_ANTLR_LIBRARY_COMMAND}
		COMMAND
			${CMAKE_COMMAND} -E copy  $<TARGET_FILE:language_server> ${CMAKE_CURRENT_SOURCE_DIR}/bin/
		COMMAND
			npm run-script package -- -o ${GLOBAL_OUTPUT_PATH}/vscode-hlasmplugin.vsix
		COMMAND 
			${CMAKE_COMMAND} -E touch ${CHECK}
		DEPENDS
			${CHECK1} ${CMAKE_CURRENT_SOURCE_DIR} parser_library language_server
		WORKING_DIRECTORY
			${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(vsix
		ALL
		DEPENDS
			${CHECK}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
