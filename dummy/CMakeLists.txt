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

PROJECT(dummy)

# compile sources to the executable
add_executable(dummy 
	${PROJECT_SOURCE_DIR}/src/main.cpp
	)

# link executable with libraries
target_link_libraries(dummy  parser_library)

if(BUILD_TESTING)
	add_dependencies(dummy library_tests_copy)
endif()
