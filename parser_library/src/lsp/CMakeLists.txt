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
    completion_list_source.h
    document_symbol_item.cpp
    file_info.cpp
    file_info.h
    folding.cpp
    folding.h
    instruction_completions.cpp
    instruction_completions.h
    item_convertors.cpp
    item_convertors.h
    lsp_context.cpp
    lsp_context.h
    macro_info.h
    opencode_info.h
    symbol_occurrence.h
    text_data_view.cpp
    text_data_view.h
)
