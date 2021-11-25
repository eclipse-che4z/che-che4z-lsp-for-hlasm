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
	data_definition.cpp
	data_definition.h
	evaluation_context.h
	mach_expr_term.cpp
	mach_expr_term.h
	mach_expr_visitor.h
	mach_expression.cpp
	mach_expression.h
	mach_operator.cpp
	mach_operator.h
	nominal_value.cpp
	nominal_value.h
)

add_subdirectory(conditional_assembly)
