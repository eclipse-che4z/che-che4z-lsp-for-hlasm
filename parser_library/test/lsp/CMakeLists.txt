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
	analyzer_fixture.h
	instruction_hints_test.cpp
	lsp_context_copy_in_macro_test.cpp
	lsp_context_document_symbol_ord_test.cpp
	lsp_context_document_symbol_var_seq_test.cpp
	lsp_context_instr_test.cpp
	lsp_context_macro_documentation_test.cpp
	lsp_context_macro_in_opencode_test.cpp
	lsp_context_nested_macro_test.cpp
	lsp_context_ord_sym_test.cpp
	lsp_context_preprocessor_test.cpp
	lsp_context_seq_sym_test.cpp
	lsp_context_test_helper.h
	lsp_context_var_sym_test.cpp
	lsp_features_test.cpp
)
