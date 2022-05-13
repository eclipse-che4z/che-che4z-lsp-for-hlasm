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


# Used from external_antlr4cpp.cmake file as a workaround for the
# following bug in cmake: https://gitlab.kitware.com/cmake/cmake/-/issues/21086
# The patch step of ExternalProject_Add is executed repeatedly even when there
# is no change to CmakeLists or the external project

set(PATCH_STAMP "${CMAKE_BINARY_DIR}/patch_applied.stamp")

if(NOT EXISTS ${PATCH_STAMP})
	execute_process(COMMAND ${GIT_EXECUTABLE} apply ${PROJECT_SOURCE_DIR}/cmake/antlr_patch.diff)
	execute_process(COMMAND ${CMAKE_COMMAND} -E touch ${PATCH_STAMP})
endif()
