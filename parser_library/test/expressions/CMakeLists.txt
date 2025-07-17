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
    arithmetic_expression_test.cpp
    ca_constant_test.cpp
    ca_expr_list_test.cpp
    ca_function_test.cpp
    ca_operator_test.cpp
    ca_string_test.cpp
    ca_sysattra_test.cpp
    ca_sysattrp_test.cpp
    ca_symbol_attribute_test.cpp
    ca_symbol_test.cpp
    ca_var_sym_test.cpp
    character_expression_test.cpp
    logical_expression_test.cpp
)

