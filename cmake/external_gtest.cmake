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

include(FetchContent)

set(INSTALL_GTEST Off)

message("Populating googletest")
FetchContent_Declare(googletest
    GIT_REPOSITORY      https://github.com/google/googletest.git
    GIT_TAG             v1.17.0
    LOG_DOWNLOAD        ON
    GIT_PROGRESS        1
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(googletest)
