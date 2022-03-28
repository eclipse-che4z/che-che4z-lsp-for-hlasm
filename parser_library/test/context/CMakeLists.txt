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
	address_test.cpp
	context_test.cpp
	data_attribute_test.cpp
	dependency_collector_test.cpp
	literals_test.cpp
	macro_test.cpp
	ord_sym_test.cpp
	system_variable_subscripts_test.cpp
	system_variable_test.cpp
	using_test.cpp
)

