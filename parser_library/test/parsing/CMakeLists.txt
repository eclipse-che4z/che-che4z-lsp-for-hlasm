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

target_sources(library_test PRIVATE
    deferred_statement_test.cpp
    label_parsing_test.cpp
    parser_model_test.cpp
    parser_range_test.cpp
    parser_test.cpp
    special_test.cpp
    string_test.cpp
)

