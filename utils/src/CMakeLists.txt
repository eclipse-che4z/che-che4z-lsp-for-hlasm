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

target_sources(hlasm_utils PRIVATE
    insist.cpp
    content_loader.cpp
    encoding.cpp
    filesystem_content_loader.cpp
    path_conversions.cpp
    platform.cpp
    resource_location.cpp
    string_operations.cpp
    unicode_text.cpp
    time.cpp
)

if(EMSCRIPTEN)
    target_sources(hlasm_utils PRIVATE emscripten_path.cpp)
else()
    target_sources(hlasm_utils PRIVATE native_path.cpp)
endif()
