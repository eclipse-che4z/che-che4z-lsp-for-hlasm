/*
 * Copyright (c) 2021 Broadcom.
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

#include <array>
#include <map>
#include <set>
#include <string>

#include "gtest/gtest.h"

#include "../common_testing.h"
#include "context/using.h"

namespace {
struct test_context : public dependency_solver
{
    id_storage m_ids;
    std::map<std::string, section> m_sect;
    std::map<id_index, symbol> m_symbols;
    std::optional<address> m_loctr;

    id_index id(const std::string& s) { return s.empty() ? nullptr : m_ids.add(s); }
    const section* section(const std::string& s)
    {
        const auto i = id(s);
        auto result = &m_sect.try_emplace(s, i, section_kind::EXECUTABLE, m_ids).first->second;
        m_symbols.try_emplace(
            i, i, address({ result }, 0, {}), symbol_attributes(symbol_origin::SECT), location(), processing_stack_t());

        return result;
    }

    address addr(const std::string& name, const std::string& sect, int offset)
    {
        address result({ section(sect) }, offset, {});
        m_symbols.try_emplace(
            id(name), id(name), result, symbol_attributes(symbol_origin::EQU), location(), processing_stack_t());
        return result;
    }

    std::unique_ptr<mach_expression> symbol(const std::string& n, const std::string& q = "")
    {
        return std::make_unique<mach_expr_symbol>(id(n), id(q), range());
    }

    std::unique_ptr<mach_expression> number(int n) const { return std::make_unique<mach_expr_constant>(n, range()); }

    std::unique_ptr<mach_expression> loctr() const { return std::make_unique<mach_expr_location_counter>(range()); }

    // Inherited via dependency_solver
    virtual const context::symbol* get_symbol(id_index name) const override { return &m_symbols.at(name); }
    virtual std::optional<address> get_loctr() const override { return m_loctr; }
    virtual id_index get_literal_id(const std::string&,
        const std::shared_ptr<const expressions::data_definition>&,
        const range& r,
        bool align_on_halfword) override
    {
        assert(false);
        return id_index();
    }
};

std::unique_ptr<mach_expression> operator+(std::unique_ptr<mach_expression> l, std::unique_ptr<mach_expression> r)
{
    return std::make_unique<mach_expr_binary<add>>(std::move(l), std::move(r), range());
}
std::unique_ptr<mach_expression> operator-(std::unique_ptr<mach_expression> l, std::unique_ptr<mach_expression> r)
{
    return std::make_unique<mach_expr_binary<sub>>(std::move(l), std::move(r), range());
}

using evaluate_result = using_collection::evaluate_result;
constexpr auto invalid_register = using_collection::invalid_register;

} // namespace

TEST(using, basic)
{
    test_context c;

    using_collection coll;
    using_collection::index_t current;
    diagnostic_op_consumer_container d;

    std::array mapping { c.number(1) };
    auto sect = c.section("SECT");

    auto with_sect = coll.add(current, nullptr, c.symbol("SECT"), nullptr, mapping, d);

    EXPECT_TRUE(d.diags.empty());

    coll.resolve_all(c, d);

    EXPECT_TRUE(d.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 0, false), evaluate_result(1, 0));
    EXPECT_EQ(coll.evaluate(with_sect, c.id("LABEL"), sect, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 1000, false), evaluate_result(1, 1000));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 4096, false), evaluate_result(invalid_register, 1));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, -1, false), evaluate_result(invalid_register, -1));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 4096, true), evaluate_result(1, 4096));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, -1, true), evaluate_result(1, -1));
}

TEST(using, multiple_registers)
{
    test_context c;

    using_collection coll;
    using_collection::index_t current;
    diagnostic_op_consumer_container d;

    std::array mapping { c.number(2), c.number(1) };
    auto sect = c.section("SECT");

    auto with_sect = coll.add(current, nullptr, c.symbol("SECT"), nullptr, mapping, d);

    EXPECT_TRUE(d.diags.empty());

    coll.resolve_all(c, d);

    EXPECT_TRUE(d.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 0, false), evaluate_result(2, 0));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 4096, false), evaluate_result(1, 0));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 4100, false), evaluate_result(1, 4));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 8192, false), evaluate_result(invalid_register, 1));

    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 0, true), evaluate_result(2, 0));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 4096, true), evaluate_result(1, 0));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 4100, true), evaluate_result(1, 4));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 8192, true), evaluate_result(1, 4096));
}

TEST(using, with_offset)
{
    test_context c;

    using_collection coll;
    using_collection::index_t current;
    diagnostic_op_consumer_container d;

    std::array mapping { c.number(2) };
    auto sect = c.section("SECT");

    auto with_sect = coll.add(current, nullptr, c.symbol("SECT") + c.number(10), nullptr, mapping, d);

    EXPECT_TRUE(d.diags.empty());

    coll.resolve_all(c, d);

    EXPECT_TRUE(d.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 0, false), evaluate_result(invalid_register, -10));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 10, false), evaluate_result(2, 0));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 20, false), evaluate_result(2, 10));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 4106, false), evaluate_result(invalid_register, 1));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 0, true), evaluate_result(2, -10));
}

TEST(using, with_negative_offset)
{
    test_context c;

    using_collection coll;
    using_collection::index_t current;
    diagnostic_op_consumer_container d;

    std::array mapping { c.number(2) };
    auto sect = c.section("SECT");

    auto with_sect = coll.add(current, nullptr, c.symbol("SECT") - c.number(10), nullptr, mapping, d);

    EXPECT_TRUE(d.diags.empty());

    coll.resolve_all(c, d);

    EXPECT_TRUE(d.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, -20, false), evaluate_result(invalid_register, -10));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, -10, false), evaluate_result(2, 0));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 0, false), evaluate_result(2, 10));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 10, false), evaluate_result(2, 20));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 20, false), evaluate_result(2, 30));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 4086, false), evaluate_result(invalid_register, 1));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, -20, true), evaluate_result(2, -10));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, -10, true), evaluate_result(2, 0));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 0, true), evaluate_result(2, 10));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 4086, false), evaluate_result(invalid_register, 1));
}

TEST(using, dependent_using)
{
    test_context c;

    using_collection coll;
    using_collection::index_t current;
    diagnostic_op_consumer_container d;

    std::array mapping { c.number(12) };
    auto sect = c.section("SECT");
    std::array mapping2 { c.symbol("SECT") + c.number(20) };
    auto sect2 = c.section("SECT2");
    /*
     * USING SECT+10,12
     * USING SECT2+5,SECT+20
     */

    auto with_sect = coll.add(current, nullptr, c.symbol("SECT") + c.number(10), nullptr, mapping, d);
    auto with_sect2 = coll.add(with_sect, nullptr, c.symbol("SECT2") + c.number(5), nullptr, mapping2, d);

    EXPECT_TRUE(d.diags.empty());

    coll.resolve_all(c, d);

    EXPECT_TRUE(d.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 10, false), evaluate_result(12, 0));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 20, false), evaluate_result(12, 10));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect2, 20, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, nullptr, sect2, 20, false), evaluate_result(12, 25));

    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 4105, false), evaluate_result(12, 4095));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 4106, false), evaluate_result(invalid_register, 1));
    EXPECT_EQ(coll.evaluate(with_sect2, nullptr, sect, 4105, false), evaluate_result(12, 4095));
    EXPECT_EQ(coll.evaluate(with_sect2, nullptr, sect, 4106, false), evaluate_result(invalid_register, 1));

    EXPECT_EQ(coll.evaluate(with_sect2, nullptr, sect2, 4090, false), evaluate_result(12, 4095));
    EXPECT_EQ(coll.evaluate(with_sect2, nullptr, sect2, 4091, false), evaluate_result(invalid_register, 1));
}

TEST(using, labeled)
{
    test_context c;

    using_collection coll;
    using_collection::index_t current;
    diagnostic_op_consumer_container d;

    std::array mapping { c.number(1) };
    auto sect = c.section("SECT");
    auto label = c.id("LABEL");

    auto with_sect = coll.add(current, label, c.symbol("SECT"), nullptr, mapping, d);

    EXPECT_TRUE(d.diags.empty());

    coll.resolve_all(c, d);

    EXPECT_TRUE(d.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect, label, sect, 0, false), evaluate_result(1, 0));
    EXPECT_EQ(coll.evaluate(with_sect, label, sect, 1000, false), evaluate_result(1, 1000));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 1000, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect, label, sect, 4096, false), evaluate_result(invalid_register, 1));
    EXPECT_EQ(coll.evaluate(with_sect, label, sect, -1, false), evaluate_result(invalid_register, -1));
    EXPECT_EQ(coll.evaluate(with_sect, label, sect, 4096, true), evaluate_result(1, 4096));
    EXPECT_EQ(coll.evaluate(with_sect, label, sect, -1, true), evaluate_result(1, -1));
}

TEST(using, drop_one)
{
    test_context c;

    using_collection coll;
    using_collection::index_t current;
    diagnostic_op_consumer_container d;

    std::array mapping { c.number(2), c.number(1) };
    auto sect = c.section("SECT");

    auto with_sect = coll.add(current, nullptr, c.symbol("SECT"), nullptr, mapping, d);

    std::array drop { c.number(2) };
    auto after_drop2 = coll.remove(with_sect, drop, d);

    EXPECT_TRUE(d.diags.empty());

    coll.resolve_all(c, d);

    EXPECT_TRUE(d.diags.empty());

    EXPECT_EQ(coll.evaluate(after_drop2, nullptr, sect, 0, false), evaluate_result(invalid_register, -4096));
    EXPECT_EQ(coll.evaluate(after_drop2, nullptr, sect, 4096, false), evaluate_result(1, 0));
    EXPECT_EQ(coll.evaluate(after_drop2, nullptr, sect, 4100, false), evaluate_result(1, 4));
    EXPECT_EQ(coll.evaluate(after_drop2, nullptr, sect, 8192, false), evaluate_result(invalid_register, 1));

    EXPECT_EQ(coll.evaluate(after_drop2, nullptr, sect, 0, true), evaluate_result(1, -4096));
    EXPECT_EQ(coll.evaluate(after_drop2, nullptr, sect, 4096, true), evaluate_result(1, 0));
    EXPECT_EQ(coll.evaluate(after_drop2, nullptr, sect, 4100, true), evaluate_result(1, 4));
    EXPECT_EQ(coll.evaluate(after_drop2, nullptr, sect, 8192, true), evaluate_result(1, 4096));
}

TEST(using, drop_dependent)
{
    test_context c;

    using_collection coll;
    using_collection::index_t current;
    diagnostic_op_consumer_container d;

    std::array mapping { c.number(2) };
    auto sect = c.section("SECT");
    std::array dep { c.symbol("SECT") };
    auto sect2 = c.section("SECT2");

    auto with_sect = coll.add(current, nullptr, c.symbol("SECT"), nullptr, mapping, d);
    auto with_sect2 = coll.add(with_sect, nullptr, c.symbol("SECT2"), nullptr, dep, d);

    std::array drop { c.number(2) };
    auto after_drop2 = coll.remove(with_sect2, drop, d);

    EXPECT_TRUE(d.diags.empty());

    coll.resolve_all(c, d);

    EXPECT_TRUE(d.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 0, false), evaluate_result(2, 0));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect2, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, nullptr, sect, 0, false), evaluate_result(2, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, nullptr, sect2, 0, false), evaluate_result(2, 0));

    EXPECT_EQ(coll.evaluate(after_drop2, nullptr, sect, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(after_drop2, nullptr, sect2, 0, false), evaluate_result(invalid_register, 0));
}

TEST(using, override_label)
{
    test_context c;

    using_collection coll;
    using_collection::index_t current;
    diagnostic_op_consumer_container d;

    std::array mapping { c.number(1) };
    auto sect = c.section("SECT");
    std::array mapping2 { c.number(1) };
    auto sect2 = c.section("SECT2");
    auto label = c.id("LABEL");

    auto with_sect = coll.add(current, label, c.symbol("SECT"), nullptr, mapping, d);
    auto with_sect2 = coll.add(with_sect, label, c.symbol("SECT2"), nullptr, mapping2, d);

    EXPECT_TRUE(d.diags.empty());

    coll.resolve_all(c, d);

    EXPECT_TRUE(d.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, nullptr, sect, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect2, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, nullptr, sect2, 0, false), evaluate_result(invalid_register, 0));

    EXPECT_EQ(coll.evaluate(with_sect, label, sect, 0, false), evaluate_result(1, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, label, sect, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect, label, sect2, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, label, sect2, 0, false), evaluate_result(1, 0));
}

TEST(using, drop_reg_with_labeled_dependent)
{
    test_context c;

    using_collection coll;
    using_collection::index_t current;
    diagnostic_op_consumer_container d;

    auto label = c.id("LABEL");
    std::array mapping { c.number(2) };
    auto sect = c.section("SECT");
    std::array dep { c.symbol("SECT") };
    auto sect2 = c.section("SECT2");

    auto with_sect = coll.add(current, nullptr, c.symbol("SECT"), nullptr, mapping, d);
    auto with_sect2 = coll.add(with_sect, label, c.symbol("SECT2"), nullptr, dep, d);

    std::array drop { c.number(2) };
    auto after_drop2 = coll.remove(with_sect2, drop, d);

    EXPECT_TRUE(d.diags.empty());

    coll.resolve_all(c, d);

    EXPECT_TRUE(d.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 0, false), evaluate_result(2, 0));
    EXPECT_EQ(coll.evaluate(with_sect, label, sect, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect2, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect, label, sect2, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, nullptr, sect, 0, false), evaluate_result(2, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, label, sect, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, nullptr, sect2, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, label, sect2, 0, false), evaluate_result(2, 0));

    EXPECT_EQ(coll.evaluate(after_drop2, nullptr, sect, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(after_drop2, label, sect2, 0, false), evaluate_result(2, 0));
}
