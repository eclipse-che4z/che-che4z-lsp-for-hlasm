cmake_minimum_required (VERSION 3.5)

PROJECT(vscode-hlasmplugin)

add_custom_target(npmInstall
		
		COMMAND
			npm install
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(vsix
		ALL
		COMMAND
			npm run-script package -- -o ${GLOBAL_OUTPUT_PATH}/vscode-hlasmplugin.vsix
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_dependencies(vsix npmInstall)