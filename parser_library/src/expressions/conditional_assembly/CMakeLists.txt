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
	ca_expr_policy.cpp
	ca_expr_policy.h
	ca_expr_visitor.h
	ca_expression.cpp
	ca_expression.h
	ca_operator_binary.cpp
	ca_operator_binary.h
	ca_operator_unary.cpp
	ca_operator_unary.h
)

add_subdirectory(terms)
