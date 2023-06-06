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

target_sources(hlasm_utils PUBLIC
	async_busy_wait.h
	bk_tree.h
	content_loader.h
	encoding.h
	error_codes.h
	factory.h
	filesystem_content_loader.h
	general_hashers.h
	levenshtein_distance.h
	list_directory_rc.h
	path.h
	path_conversions.h
	platform.h
	resource_location.h
	scope_exit.h
	similar.h
	string_operations.h
	task.h
	text_matchers.h
	time.h
	transform_inserter.h
	truth_table.h
	unicode_text.h
)
