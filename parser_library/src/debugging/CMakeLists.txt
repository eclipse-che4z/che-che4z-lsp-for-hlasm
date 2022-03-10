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
	attribute_variable.cpp
	attribute_variable.h
	debug_lib_provider.h
	debug_types.h
	debugger.cpp
	macro_param_variable.cpp
	macro_param_variable.h
	ordinary_symbol_variable.cpp
	ordinary_symbol_variable.h
	set_symbol_variable.cpp
	set_symbol_variable.h
	variable.h
)

