cmake_minimum_required (VERSION 3.10)

PROJECT(vscode-hlasmplugin)

set(CHECK "${GLOBAL_OUTPUT_PATH}/npm_install.stamp")

if (WIN32)
	set(CALL "call")
else()
	set(CALL "")
endif()

add_custom_command(
		OUTPUT
			${CHECK}
		COMMAND
			${CALL} npm install
		COMMAND
			${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/bin/
		COMMAND
			${CMAKE_COMMAND} -E copy  $<TARGET_FILE:parser_library> ${CMAKE_CURRENT_SOURCE_DIR}/bin/
		COMMAND
			${CMAKE_COMMAND} -E copy  ${GLOBAL_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}antlr4-runtime${CMAKE_SHARED_LIBRARY_SUFFIX} ${CMAKE_CURRENT_SOURCE_DIR}/bin/
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
