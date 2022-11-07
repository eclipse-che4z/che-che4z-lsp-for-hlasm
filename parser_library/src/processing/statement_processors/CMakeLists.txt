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
	copy_processing_info.h
	copy_processor.cpp
	copy_processor.h
	empty_processor.cpp
	empty_processor.h
	lookahead_processing_info.h
	lookahead_processor.cpp
	lookahead_processor.h
	macrodef_processing_info.h
	macrodef_processor.cpp
	macrodef_processor.h
	ordinary_processor.cpp
	ordinary_processor.h
	statement_processor.h
)

