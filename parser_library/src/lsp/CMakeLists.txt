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
	completion_item.cpp
	completion_item.h
	feature_provider.h
	file_info.cpp
	file_info.h
	lsp_context.cpp
	lsp_context.h
	macro_info.h
	opencode_info.h
	symbol_occurence.h
	text_data_ref_t.cpp
	text_data_ref_t.h
)
