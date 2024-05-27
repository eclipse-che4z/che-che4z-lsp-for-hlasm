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
    FILE_SET public_headers TYPE HEADERS FILES
    utils/async_busy_wait.h
    utils/bk_tree.h
    utils/content_loader.h
    utils/encoding.h
    utils/error_codes.h
    utils/factory.h
    utils/filesystem_content_loader.h
    utils/filter_vector.h
    utils/general_hashers.h
    utils/levenshtein_distance.h
    utils/list_directory_rc.h
    utils/merge_sorted.h
    utils/path.h
    utils/path_conversions.h
    utils/platform.h
    utils/projectors.h
    utils/resource_location.h
    utils/scope_exit.h
    utils/similar.h
    utils/string_operations.h
    utils/task.h
    utils/text_matchers.h
    utils/time.h
    utils/transform_inserter.h
    utils/truth_table.h
    utils/unicode_text.h
)
