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

#include "gtest/gtest.h"

#include "../common_testing.h"
#include "context/hlasm_context.h"
#include "context/ordinary_assembly/location_counter.h"
#include "context/ordinary_assembly/ordinary_assembly_dependency_solver.h"
#include "expressions/mach_expr_term.h"
#include "expressions/mach_operator.h"
#include "library_info_transitional.h"

// test for
// dependency_collector class

TEST(dependency_collector, uresolved_addresses)
{
    hlasm_context ctx;
    id_index name1("SYM1");
    id_index name2("SYM2");

    ctx.ord_ctx.set_section(id_index("TEST"), section_kind::COMMON, location(), library_info_transitional::empty);
    auto addr1 = ctx.ord_ctx.current_section()->current_location_counter().current_address();

    (void)ctx.ord_ctx.create_symbol(name1,
        symbol_value(std::move(addr1)),
        symbol_attributes(symbol_origin::UNKNOWN),
        location(),
        library_info_transitional::empty);

    auto sp = ctx.ord_ctx.current_section()->current_location_counter().register_ordinary_space(halfword);
    auto addr2 = ctx.ord_ctx.current_section()->current_location_counter().current_address();

    (void)ctx.ord_ctx.create_symbol(name2,
        symbol_value(std::move(addr2)),
        symbol_attributes(symbol_origin::UNKNOWN),
        location(),
        library_info_transitional::empty);

    // ((SYM2-SYM1)/(SYM2-SYM1))+SYM2
    mach_expr_binary<add> expr(std::make_unique<mach_expr_binary<expressions::div>>(
                                   std::make_unique<mach_expr_binary<expressions::sub>>(
                                       std::make_unique<mach_expr_symbol>(name2, id_index(), range()),
                                       std::make_unique<mach_expr_symbol>(name1, id_index(), range()),
                                       range()),
                                   std::make_unique<mach_expr_binary<expressions::sub>>(
                                       std::make_unique<mach_expr_symbol>(name2, id_index(), range()),
                                       std::make_unique<mach_expr_symbol>(name1, id_index(), range()),
                                       range()),
                                   range()),
        std::make_unique<mach_expr_symbol>(name2, id_index(), range()),
        range());

    context::ordinary_assembly_dependency_solver dep_solver(ctx.ord_ctx, library_info_transitional::empty);

    auto deps = expr.get_dependencies(dep_solver);

    ASSERT_EQ(deps.unresolved_spaces.size(), (size_t)1);
    EXPECT_TRUE(std::ranges::find(deps.unresolved_spaces, sp) != deps.unresolved_spaces.end());

    deps.unresolved_address->normalized_spaces();
}
