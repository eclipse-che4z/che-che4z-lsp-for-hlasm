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
	data_definition_check_test.cpp
	data_definition_common.h
	data_definition_integer_test.cpp
	data_definition_length_test.cpp
	data_definition_scale_test.cpp
	data_definition_test.cpp
)

