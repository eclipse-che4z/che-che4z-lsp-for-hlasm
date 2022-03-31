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
	diags_suppress_test.cpp
	empty_configs.h
	extension_handling_test.cpp
	file_with_text.h
	files_parse_lib_provider.h
	load_config_test.cpp
	macro_cache_test.cpp
	pathmask_test.cpp
	processor_file_test.cpp
	processor_group_test.cpp
	text_synchronization_test.cpp
	virtual_files_test.cpp
	workspace_test.cpp
)

