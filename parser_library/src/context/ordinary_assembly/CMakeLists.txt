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
    address.cpp
    address.h
    address_resolver.cpp
    address_resolver.h
    alignment.h
    dependable.h
    dependant.cpp
    dependant.h
    dependency_collector.cpp
    dependency_collector.h
    dependency_solver_redirect.cpp
    dependency_solver_redirect.h
    location_counter.cpp
    location_counter.h
    location_counter_data.cpp
    location_counter_data.h
    ordinary_assembly_context.cpp
    ordinary_assembly_context.h
    ordinary_assembly_dependency_solver.cpp
    ordinary_assembly_dependency_solver.h
    postponed_statement.cpp
    postponed_statement.h
    section.cpp
    section.h
    symbol.cpp
    symbol.h
    symbol_attributes.cpp
    symbol_attributes.h
    symbol_dependency_tables.cpp
    symbol_dependency_tables.h
    symbol_value.cpp
    symbol_value.h
)

