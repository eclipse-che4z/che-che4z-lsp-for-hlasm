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

target_sources(hlasm_language_server_base PRIVATE
	feature_language_features.cpp
	feature_language_features.h
	feature_text_synchronization.cpp
	feature_text_synchronization.h
	feature_workspace_folders.cpp
	feature_workspace_folders.h
	lsp_server.cpp
	lsp_server.h
)
