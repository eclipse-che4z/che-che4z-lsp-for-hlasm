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
	collector.cpp
	collector.h
	concatenation.cpp
	concatenation.h
	concatenation_term.cpp
	concatenation_term.h
	highlighting_info.h
	operand.h
	operand_impls.cpp
	operand_impls.h
	operand_visitor.h
	range_provider.cpp
	range_provider.h
	source_info_processor.cpp
	source_info_processor.h
	statement.h
	statement_fields.h
	variable_symbol.cpp
	variable_symbol.h
)
