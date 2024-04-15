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

target_sources(parser_library PRIVATE
    ca_constant.cpp
    ca_constant.h
    ca_expr_list.cpp
    ca_expr_list.h
    ca_function.cpp
    ca_function.h
    ca_string.cpp
    ca_string.h
    ca_symbol.cpp
    ca_symbol.h
    ca_symbol_attribute.cpp
    ca_symbol_attribute.h
    ca_var_sym.cpp
    ca_var_sym.h
)

