# Copyright (c) 2021 Broadcom.
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

include(GoogleTest)

add_library(hlasm_utils STATIC EXCLUDE_FROM_ALL)

target_include_directories(hlasm_utils PUBLIC include)

add_subdirectory(include)
add_subdirectory(src)

set_property(TARGET hlasm_utils PROPERTY VERIFY_INTERFACE_HEADER_SETS ON)

# link executable with libraries
target_link_libraries(hlasm_utils PRIVATE network-uri)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
