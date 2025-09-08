/*
 * Copyright (c) 2019 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Broadcom, Inc. - initial API and implementation
 */

#ifndef CONTEXT_ADDRESS_RESOLVER_H
#define CONTEXT_ADDRESS_RESOLVER_H

#include "address.h"
#include "dependable.h"

namespace hlasm_plugin::parser_library::context {

struct address_resolver_base : public resolvable
{
    virtual ~address_resolver_base() = default;
};

using addr_res_ptr = std::unique_ptr<address_resolver_base>;

// structure wrapping address providing resolvable interface to it
struct address_resolver : public address_resolver_base
{
    explicit address_resolver(address dependency_address, size_t boundary);

    dependency_collector get_dependencies(dependency_solver& solver) const override;

    symbol_value resolve(dependency_solver& solver) const override;

    static address extract_dep_address(const address& addr, size_t boundary);

protected:
    address dependency_address;
    size_t boundary;
};

// provides resolvable interface for address that require certain alignment
struct alignable_address_resolver final : public address_resolver
{
    alignable_address_resolver(address dependency_address, address base_addrs, size_t boundary, int offset);

    symbol_value resolve(dependency_solver& solver) const override;

private:
    address base_addrs;
    int offset;
    symbol_value resolve(const address& addr) const;
};

// provides resolvable interface for the agregate of addresses
struct aggregate_address_resolver final : public address_resolver_base
{
    aggregate_address_resolver(std::vector<address> base_addrs, size_t boundary, int offset);

    symbol_value resolve(dependency_solver& solver) const override;

    dependency_collector get_dependencies(dependency_solver& solver) const override;

private:
    mutable size_t last_base_addrs;
    std::vector<address> base_addrs;
    size_t boundary;
    int offset;
};

// provides resolvable interface for absolute part of the address
struct alignable_address_abs_part_resolver final : public address_resolver_base
{
    alignable_address_abs_part_resolver(const resolvable* dependency_source);

    dependency_collector get_dependencies(dependency_solver& solver) const override;

    symbol_value resolve(dependency_solver& solver) const override;

private:
    const resolvable* dependency_source_;
};

} // namespace hlasm_plugin::parser_library::context

#endif
