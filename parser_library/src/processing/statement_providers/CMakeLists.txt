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
	copy_statement_provider.cpp
	copy_statement_provider.h
	macro_statement_provider.cpp
	macro_statement_provider.h
	members_statement_provider.cpp
	members_statement_provider.h
	statement_provider.cpp
	statement_provider.h
	statement_provider_kind.h
)

