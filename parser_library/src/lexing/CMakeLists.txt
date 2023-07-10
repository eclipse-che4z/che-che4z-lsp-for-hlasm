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
	input_source.cpp
	input_source.h
	lexer.cpp
	lexer.h
	logical_line.cpp
	logical_line.h
	token.cpp
	token.h
	token_stream.cpp
	token_stream.h
	tools.cpp
	tools.h
)

