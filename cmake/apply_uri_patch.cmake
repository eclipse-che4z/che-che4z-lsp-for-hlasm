# Copyright (c) 2023 Broadcom.
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

set(PATCH_STAMP "${CMAKE_BINARY_DIR}/uri_patch_applied.stamp")

if(NOT EXISTS ${PATCH_STAMP})
    execute_process(COMMAND ${GIT_EXECUTABLE} apply ${PROJECT_SOURCE_DIR}/cmake/uri_patch.diff)
    execute_process(COMMAND ${CMAKE_COMMAND} -E touch ${PATCH_STAMP})
endif()
