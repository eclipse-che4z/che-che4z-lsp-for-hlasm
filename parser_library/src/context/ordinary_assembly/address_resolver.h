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

namespace hlasm_plugin {
namespace parser_library {
namespace context {

struct address_resolver_base : public resolvable
{};

using addr_res_ptr = std::unique_ptr<address_resolver_base>;

// structure wrapping address providing resolvable interface to it
struct address_resolver : public address_resolver_base
{
    explicit address_resolver(address dependency_address);

    dependency_collector get_dependencies(dependency_solver& solver) const override;

    symbol_value resolve(dependency_solver& solver) const override;

protected:
    address dependency_address;
    dependency_collector cached_deps_;
    static address extract_dep_address(const address& addr);
};

// provides resolvable interface for address that require certain alignment
struct alignable_address_resolver : public address_resolver
{
    alignable_address_resolver(
        address dependency_address, std::vector<address> base_addrs, size_t boundary, int offset);

    symbol_value resolve(dependency_solver& solver) const override;

protected:
    std::vector<address> base_addrs;
    size_t boundary;
    int offset;
    symbol_value resolve(const address& addr) const;
    alignable_address_resolver(
        address dependency_address, std::vector<address>& base_addrs, size_t boundary, int offset, bool);
};

// provides resolvable interface for the agregate of addresses
struct aggregate_address_resolver : public alignable_address_resolver
{
    aggregate_address_resolver(std::vector<address> base_addrs, size_t boundary, int offset);

    symbol_value resolve(dependency_solver& solver) const override;
};

// provides resolvable interface for absolute part of the address
struct alignable_address_abs_part_resolver : public address_resolver_base
{
    alignable_address_abs_part_resolver(const resolvable* dependency_source);

    dependency_collector get_dependencies(dependency_solver& solver) const override;

    symbol_value resolve(dependency_solver& solver) const override;

private:
    const resolvable* dependency_source_;
};

} // namespace context
} // namespace parser_library
} // namespace hlasm_plugin

#endif
