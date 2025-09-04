/*
 * Copyright (c) 2022 Broadcom.
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
#include "context/hlasm_context.h"
#include "context/ordinary_assembly/location_counter.h"
#include "context/using.h"
#include "expressions/mach_expr_term.h"
#include "expressions/mach_operator.h"
#include "library_info_transitional.h"

namespace {
struct test_context : public dependency_solver
{
    hlasm_context hlasm_ctx;
    ordinary_assembly_context asm_ctx;
    test_context()
        : asm_ctx(hlasm_ctx)
    {}

    std::map<std::string, section> m_sect;
    std::map<id_index, symbol> m_symbols;
    std::optional<address> m_loctr;

    id_index id(const std::string& s) { return hlasm_ctx.add_id(s); }
    id_index label(const std::string& s)
    {
        auto label = hlasm_ctx.add_id(s);
        asm_ctx.register_using_label(label);
        return label;
    }
    const section* create_section(const std::string& s)
    {
        return asm_ctx.set_section(id(s), section_kind::COMMON, library_info_transitional::empty);
    }

    address addr(const std::string& name, const std::string& sect, int offset)
    {
        address result({ {}, create_section(sect) }, offset, {});
        m_symbols.try_emplace(id(name), id(name), result, symbol_attributes(symbol_origin::EQU), processing_stack_t());
        return result;
    }

    std::unique_ptr<mach_expression> create_symbol(const std::string& n, const std::string& q = "")
    {
        return std::make_unique<mach_expr_symbol>(id(n), q.empty() ? id_index() : id(q), range());
    }

    std::unique_ptr<mach_expression> number(int n) const { return std::make_unique<mach_expr_constant>(n, range()); }

    std::unique_ptr<mach_expression> loctr() const { return std::make_unique<mach_expr_location_counter>(range()); }

    // Inherited via dependency_solver
    const context::symbol* get_symbol(id_index name) const override { return &m_symbols.at(name); }
    std::optional<address> get_loctr() const override { return m_loctr; }
    id_index get_literal_id(const std::shared_ptr<const expressions::data_definition>&) override
    {
        assert(false);
        return id_index();
    }
    bool using_active(id_index /*label*/, const section* /*sect*/) const override
    {
        assert(false);
        return false;
    }
    using_evaluate_result using_evaluate(
        id_index /*label*/, const section* /*owner*/, int32_t /*offset*/, bool /*long_offset*/) const override
    {
        assert(false);
        return { using_collection::invalid_register, 0 };
    }
    std::variant<const context::symbol*, context::symbol_candidate> get_symbol_candidate(
        context::id_index name) const override
    {
        return get_symbol(name);
    }
    std::string get_opcode_attr(id_index name) const override { return hlasm_ctx.get_opcode_attr(name); }
    const asm_option& get_options() const noexcept override { return hlasm_ctx.options(); }
    const section* get_section(id_index name) const noexcept override { return asm_ctx.get_section(name); }
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

template<typename... T>
std::vector<mach_expr_ptr> args(T... t)
{
    std::vector<mach_expr_ptr> result;
    result.reserve(sizeof...(T));
    (result.push_back(std::move(t)), ...);
    return result;
}

} // namespace

TEST(using, basic)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect = coll.add(current,
        id_index(),
        c.create_symbol("SECT"),
        nullptr,
        args(c.number(1)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(d_s.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 0, false), evaluate_result(1, 0));
    EXPECT_EQ(coll.evaluate(with_sect, c.id("LABEL"), sect, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 1000, false), evaluate_result(1, 1000));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 4096, false), evaluate_result(invalid_register, 1));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, -1, false), evaluate_result(invalid_register, -1));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 4096, true), evaluate_result(1, 4096));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, -1, true), evaluate_result(1, -1));
}

TEST(using, multiple_registers)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect = coll.add(current,
        id_index(),
        c.create_symbol("SECT"),
        nullptr,
        args(c.number(2), c.number(1)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(d_s.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 0, false), evaluate_result(2, 0));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 4096, false), evaluate_result(1, 0));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 4100, false), evaluate_result(1, 4));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 8192, false), evaluate_result(invalid_register, 1));

    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 0, true), evaluate_result(2, 0));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 4096, true), evaluate_result(1, 0));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 4100, true), evaluate_result(1, 4));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 8192, true), evaluate_result(1, 4096));
}

TEST(using, with_offset)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect = coll.add(current,
        id_index(),
        c.create_symbol("SECT") + c.number(10),
        nullptr,
        args(c.number(2)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(d_s.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 0, false), evaluate_result(invalid_register, -10));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 10, false), evaluate_result(2, 0));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 20, false), evaluate_result(2, 10));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 4106, false), evaluate_result(invalid_register, 1));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 0, true), evaluate_result(2, -10));
}

TEST(using, with_negative_offset)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect = coll.add(current,
        id_index(),
        c.create_symbol("SECT") - c.number(10),
        nullptr,
        args(c.number(2)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(d_s.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, -20, false), evaluate_result(invalid_register, -10));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, -10, false), evaluate_result(2, 0));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 0, false), evaluate_result(2, 10));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 10, false), evaluate_result(2, 20));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 20, false), evaluate_result(2, 30));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 4086, false), evaluate_result(invalid_register, 1));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, -20, true), evaluate_result(2, -10));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, -10, true), evaluate_result(2, 0));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 0, true), evaluate_result(2, 10));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 4086, false), evaluate_result(invalid_register, 1));
}

TEST(using, dependent_using)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    auto sect = c.create_section("SECT");
    auto sect2 = c.create_section("SECT2");
    /*
     * USING SECT+10,12
     * USING SECT2+5,SECT+20
     */

    [[maybe_unused]] auto with_sect = coll.add(current,
        id_index(),
        c.create_symbol("SECT") + c.number(10),
        nullptr,
        args(c.number(12)),
        dependency_evaluation_context(opcode_generation::current),
        {});
    [[maybe_unused]] auto with_sect2 = coll.add(with_sect,
        id_index(),
        c.create_symbol("SECT2") + c.number(5),
        nullptr,
        args(c.create_symbol("SECT") + c.number(20)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(d_s.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 10, false), evaluate_result(12, 0));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 20, false), evaluate_result(12, 10));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect2, 20, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, id_index(), sect2, 20, false), evaluate_result(12, 25));

    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 4105, false), evaluate_result(12, 4095));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 4106, false), evaluate_result(invalid_register, 1));
    EXPECT_EQ(coll.evaluate(with_sect2, id_index(), sect, 4105, false), evaluate_result(12, 4095));
    EXPECT_EQ(coll.evaluate(with_sect2, id_index(), sect, 4106, false), evaluate_result(invalid_register, 1));

    EXPECT_EQ(coll.evaluate(with_sect2, id_index(), sect2, 4090, false), evaluate_result(12, 4095));
    EXPECT_EQ(coll.evaluate(with_sect2, id_index(), sect2, 4091, false), evaluate_result(invalid_register, 1));
}

TEST(using, labeled)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    auto sect = c.create_section("SECT");
    auto label = c.id("LABEL");

    [[maybe_unused]] auto with_sect = coll.add(current,
        label,
        c.create_symbol("SECT"),
        nullptr,
        args(c.number(1)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(d_s.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect, label, sect, 0, false), evaluate_result(1, 0));
    EXPECT_EQ(coll.evaluate(with_sect, label, sect, 1000, false), evaluate_result(1, 1000));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 1000, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect, label, sect, 4096, false), evaluate_result(invalid_register, 1));
    EXPECT_EQ(coll.evaluate(with_sect, label, sect, -1, false), evaluate_result(invalid_register, -1));
    EXPECT_EQ(coll.evaluate(with_sect, label, sect, 4096, true), evaluate_result(1, 4096));
    EXPECT_EQ(coll.evaluate(with_sect, label, sect, -1, true), evaluate_result(1, -1));
}

TEST(using, drop_one)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect = coll.add(current,
        id_index(),
        c.create_symbol("SECT"),
        nullptr,
        args(c.number(2), c.number(1)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    [[maybe_unused]] auto after_drop2 =
        coll.remove(with_sect, args(c.number(2)), dependency_evaluation_context(opcode_generation::current), {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(d_s.diags.empty());

    EXPECT_EQ(coll.evaluate(after_drop2, id_index(), sect, 0, false), evaluate_result(invalid_register, -4096));
    EXPECT_EQ(coll.evaluate(after_drop2, id_index(), sect, 4096, false), evaluate_result(1, 0));
    EXPECT_EQ(coll.evaluate(after_drop2, id_index(), sect, 4100, false), evaluate_result(1, 4));
    EXPECT_EQ(coll.evaluate(after_drop2, id_index(), sect, 8192, false), evaluate_result(invalid_register, 1));

    EXPECT_EQ(coll.evaluate(after_drop2, id_index(), sect, 0, true), evaluate_result(1, -4096));
    EXPECT_EQ(coll.evaluate(after_drop2, id_index(), sect, 4096, true), evaluate_result(1, 0));
    EXPECT_EQ(coll.evaluate(after_drop2, id_index(), sect, 4100, true), evaluate_result(1, 4));
    EXPECT_EQ(coll.evaluate(after_drop2, id_index(), sect, 8192, true), evaluate_result(1, 4096));
}

TEST(using, drop_dependent)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    auto sect = c.create_section("SECT");
    auto sect2 = c.create_section("SECT2");

    [[maybe_unused]] auto with_sect = coll.add(current,
        id_index(),
        c.create_symbol("SECT"),
        nullptr,
        args(c.number(2)),
        dependency_evaluation_context(opcode_generation::current),
        {});
    [[maybe_unused]] auto with_sect2 = coll.add(with_sect,
        id_index(),
        c.create_symbol("SECT2"),
        nullptr,
        args(c.create_symbol("SECT")),
        dependency_evaluation_context(opcode_generation::current),
        {});

    [[maybe_unused]] auto after_drop2 =
        coll.remove(with_sect2, args(c.number(2)), dependency_evaluation_context(opcode_generation::current), {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(d_s.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 0, false), evaluate_result(2, 0));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect2, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, id_index(), sect, 0, false), evaluate_result(2, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, id_index(), sect2, 0, false), evaluate_result(2, 0));

    EXPECT_EQ(coll.evaluate(after_drop2, id_index(), sect, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(after_drop2, id_index(), sect2, 0, false), evaluate_result(invalid_register, 0));
}

TEST(using, override_label)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    auto sect = c.create_section("SECT");
    auto sect2 = c.create_section("SECT2");
    auto label = c.id("LABEL");

    [[maybe_unused]] auto with_sect = coll.add(current,
        label,
        c.create_symbol("SECT"),
        nullptr,
        args(c.number(1)),
        dependency_evaluation_context(opcode_generation::current),
        {});
    [[maybe_unused]] auto with_sect2 = coll.add(with_sect,
        label,
        c.create_symbol("SECT2"),
        nullptr,
        args(c.number(1)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(d_s.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, id_index(), sect, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect2, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, id_index(), sect2, 0, false), evaluate_result(invalid_register, 0));

    EXPECT_EQ(coll.evaluate(with_sect, label, sect, 0, false), evaluate_result(1, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, label, sect, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect, label, sect2, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, label, sect2, 0, false), evaluate_result(1, 0));
}

TEST(using, drop_reg_with_labeled_dependent)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    auto label = c.id("LABEL");
    auto sect = c.create_section("SECT");
    auto sect2 = c.create_section("SECT2");

    [[maybe_unused]] auto with_sect = coll.add(current,
        id_index(),
        c.create_symbol("SECT"),
        nullptr,
        args(c.number(2)),
        dependency_evaluation_context(opcode_generation::current),
        {});
    [[maybe_unused]] auto with_sect2 = coll.add(with_sect,
        label,
        c.create_symbol("SECT2"),
        nullptr,
        args(c.create_symbol("SECT")),
        dependency_evaluation_context(opcode_generation::current),
        {});

    [[maybe_unused]] auto after_drop2 =
        coll.remove(with_sect2, args(c.number(2)), dependency_evaluation_context(opcode_generation::current), {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(d_s.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 0, false), evaluate_result(2, 0));
    EXPECT_EQ(coll.evaluate(with_sect, label, sect, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect2, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect, label, sect2, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, id_index(), sect, 0, false), evaluate_result(2, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, label, sect, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, id_index(), sect2, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(with_sect2, label, sect2, 0, false), evaluate_result(2, 0));

    EXPECT_EQ(coll.evaluate(after_drop2, id_index(), sect, 0, false), evaluate_result(invalid_register, 0));
    EXPECT_EQ(coll.evaluate(after_drop2, label, sect2, 0, false), evaluate_result(2, 0));
}

TEST(using, no_drop_warning)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    [[maybe_unused]] auto after_drop2 =
        coll.remove(current, args(c.number(2)), dependency_evaluation_context(opcode_generation::current), {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U001" }));
}

TEST(using, use_reg_16)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    [[maybe_unused]] auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect = coll.add(current,
        id_index(),
        c.create_symbol("SECT"),
        nullptr,
        args(c.number(16)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "M120" }));
}

TEST(using, use_non_simple_reloc)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    [[maybe_unused]] auto with_sect = coll.add(current,
        id_index(),
        c.create_symbol("SECT"),
        nullptr,
        args(c.create_symbol("LABEL") + c.create_symbol("LABEL")),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "M113" }));
}

TEST(using, drop_qualified_label)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    [[maybe_unused]] auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect = coll.add(current,
        c.label("LABEL"),
        c.create_symbol("SECT"),
        nullptr,
        args(c.number(1)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    [[maybe_unused]] auto after_drop2 = coll.remove(with_sect,
        args(c.create_symbol("LABEL", "LABEL")),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U003" }));
}

TEST(using, drop_reg_16)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    [[maybe_unused]] auto after_drop2 =
        coll.remove(current, args(c.number(16)), dependency_evaluation_context(opcode_generation::current), {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U003" }));
}

TEST(using, drop_reloc)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    [[maybe_unused]] auto sect = c.create_section("LABEL");
    [[maybe_unused]] auto after_drop2 = coll.remove(current,
        args(c.create_symbol("LABEL") + c.create_symbol("LABEL")),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U003" }));
}

TEST(using, dependent_no_active_using)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    [[maybe_unused]] auto sect = c.create_section("SECT");
    [[maybe_unused]] auto sect2 = c.create_section("SECT2");

    [[maybe_unused]] auto with_sect2 = coll.add(current,
        id_index(),
        c.create_symbol("SECT2") + c.number(5),
        nullptr,
        args(c.create_symbol("SECT") + c.number(20)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U004" }));
}

TEST(using, dependent_no_active_matching_using)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    [[maybe_unused]] auto sect = c.create_section("SECT");
    [[maybe_unused]] auto sect2 = c.create_section("SECT2");
    [[maybe_unused]] auto sect3 = c.create_section("SECT3");
    /*
     * USING SECT+10,12
     * USING SECT2+5,SECT+20
     */

    [[maybe_unused]] auto with_sect = coll.add(current,
        id_index(),
        c.create_symbol("SECT") + c.number(10),
        nullptr,
        args(c.number(12)),
        dependency_evaluation_context(opcode_generation::current),
        {});
    [[maybe_unused]] auto with_sect2 = coll.add(with_sect,
        id_index(),
        c.create_symbol("SECT2") + c.number(5),
        nullptr,
        args(c.create_symbol("SECT3") + c.number(20)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U004" }));
}

TEST(using, using_undefined_begin)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    [[maybe_unused]] auto with_sect = coll.add(current,
        id_index(),
        c.create_symbol("SECT"),
        nullptr,
        args(c.number(1)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "M113" }));
}

TEST(using, using_qualified_begin)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    [[maybe_unused]] auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect = coll.add(current,
        id_index(),
        c.create_symbol("SECT", "LABEL"),
        nullptr,
        args(c.number(1)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U002" }));
}

TEST(using, using_undefined_end)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    [[maybe_unused]] auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect = coll.add(current,
        id_index(),
        c.create_symbol("SECT"),
        c.create_symbol("SECT_END"),
        args(c.number(1)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "M113" }));
}

TEST(using, using_qualified_end)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    [[maybe_unused]] auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect = coll.add(current,
        id_index(),
        c.create_symbol("SECT"),
        c.create_symbol("SECT", "LABEL") + c.number(1),
        args(c.number(1)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U002" }));
}

TEST(using, using_bad_range)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    [[maybe_unused]] auto sect = c.create_section("SECT");
    [[maybe_unused]] auto sect2 = c.create_section("SECT2");

    [[maybe_unused]] auto with_sect = coll.add(current,
        id_index(),
        c.create_symbol("SECT"),
        c.create_symbol("SECT"),
        args(c.number(1)),
        dependency_evaluation_context(opcode_generation::current),
        {});
    [[maybe_unused]] auto with_sect2 = coll.add(current,
        id_index(),
        c.create_symbol("SECT"),
        c.create_symbol("SECT2"),
        args(c.number(1)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U005", "U005" }));
}


TEST(using, use_complex_reloc)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    [[maybe_unused]] auto s1 = c.create_section("S1");
    [[maybe_unused]] auto s2 = c.create_section("S2");
    [[maybe_unused]] auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect = coll.add(current,
        id_index(),
        c.create_symbol("SECT"),
        nullptr,
        args(c.create_symbol("S1") + c.create_symbol("S2")),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "M120" }));
}

TEST(using, drop_invalid)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    [[maybe_unused]] auto after_drop2 = coll.remove(
        current, args(c.create_symbol("LABEL")), dependency_evaluation_context(opcode_generation::current), {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U003" }));
}

TEST(using, drop_inactive)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    [[maybe_unused]] auto label = c.label("LABEL");

    [[maybe_unused]] auto after_drop2 = coll.remove(
        current, args(c.create_symbol("LABEL")), dependency_evaluation_context(opcode_generation::current), {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U001" }));
}

TEST(using, basic_with_limit)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect = coll.add(current,
        id_index(),
        c.create_symbol("SECT"),
        c.create_symbol("SECT") + c.number(10),
        args(c.number(1)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(d_s.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 0, false), evaluate_result(1, 0));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 9, false), evaluate_result(1, 9));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), sect, 20, false), evaluate_result(invalid_register, 11));
}

TEST(using, duplicate_base)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    [[maybe_unused]] auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect = coll.add(current,
        id_index(),
        c.create_symbol("SECT"),
        nullptr,
        args(c.number(1), c.number(1)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U006" }));
}

TEST(using, absolute)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    [[maybe_unused]] auto with_sect = coll.add(current,
        id_index(),
        c.number(128),
        nullptr,
        args(c.number(1)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(d_s.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, id_index(), nullptr, 0, false), evaluate_result(0, 0));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), nullptr, 10, false), evaluate_result(0, 10));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), nullptr, 128, false), evaluate_result(1, 0));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), nullptr, 256, false), evaluate_result(1, 128));
    EXPECT_EQ(coll.evaluate(with_sect, id_index(), nullptr, -100, true), evaluate_result(0, -100));
}

TEST(using, smaller_offset_but_invalid)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    [[maybe_unused]] auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect = coll.add(current,
        id_index(),
        c.create_symbol("SECT"),
        nullptr,
        args(c.number(1)),
        dependency_evaluation_context(opcode_generation::current),
        {});
    [[maybe_unused]] auto with_sect2 = coll.add(with_sect,
        id_index(),
        c.create_symbol("SECT") + c.number(100),
        c.create_symbol("SECT") + c.number(120),
        args(c.number(2)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(d_s.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect2, id_index(), sect, 128, false), evaluate_result(1, 128));
}

TEST(using, describe)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic> d_s;

    auto sect = c.create_section("SECT");
    auto label = c.id("LABEL");

    const auto with_sect = coll.add(current,
        label,
        c.create_symbol("SECT") + c.number(100),
        c.create_symbol("SECT") + c.number(120),
        args(c.number(1), c.number(2)),
        dependency_evaluation_context(opcode_generation::current),
        {});

    coll.resolve_all(c.asm_ctx, d_s, library_info_transitional::empty);

    EXPECT_TRUE(d_s.diags.empty());

    const std::vector<using_context_description> expected {
        using_context_description { label, sect->name, 100, 20, 0, { 1, 2 } },
    };

    EXPECT_EQ(coll.describe(with_sect), expected);
}

TEST(using, describe_empty)
{
    using_collection coll;

    EXPECT_TRUE(coll.describe({}).empty());
}

TEST(using, simple_using)
{
    std::string input = R"(
TEST  CSECT
LABEL USING TEST,1
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(using, empty_operands)
{
    std::string input = R"(
TEST  CSECT
LABEL USING 1,
LABEL USING ,
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A164", "A104" }));
}

TEST(using, wrong_operands)
{
    std::string input = R"(
LABEL USING 1,(1,1)
LABEL USING 1,'FAIL'
LABEL USING 'FAIL',1
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002", "S0002", "S0011", "A164", "A164", "A012" }));
}

TEST(using, push_pop)
{
    std::string input = R"(
    PUSH USING
    POP  USING
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(using, pop_empty)
{
    std::string input = R"(
    POP  USING
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A165" }));
}

TEST(using, register_alias)
{
    std::string input = R"(
TEST  CSECT
      USING TEST,R1
R1    EQU   1
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(using, label_conflict_1)
{
    std::string input = R"(
TEST  CSECT
LABEL USING TEST,R1
R1    EQU   1
LABEL DS    F
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E031" }));
}

TEST(using, label_conflict_2)
{
    std::string input = R"(
LABEL DS    F
TEST  CSECT
LABEL USING TEST,R1
R1    EQU   1
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E031" }));
}

TEST(using, nested_diagnostics)
{
    std::string input = R"(
      MACRO
      MAC
      USING TEST,0-1
      MEND

TEST  CSECT
      MAC
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_properties(a.diags(), { false }, [](const auto& m) { return m.related.empty(); }));
}

TEST(using, macro_model)
{
    std::string input = R"(
      MACRO
&L    MAC   &N
&L    USING &N,1
&L._  USING TEST2,&L..&N
      MEND

TEST1 CSECT
TEST2 CSECT
LABEL MAC    TEST1
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(using, weird_constructs)
{
    std::string input = R"(
TEST  CSECT
      USING TEST,0
      DROP  =A(0)-=A(0)
      LTORG ,
TESTL EQU   *-TEST
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "TESTL"), 4);
}

TEST(using, weird_constructs_2)
{
    std::string input = R"(
TEST  CSECT
      USING TEST,12
      DROP  8-(=A(0)-=A(4))
      LTORG ,
TESTL EQU   *-TEST
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "TESTL"), 8);
}

TEST(using, self_mapping)
{
    std::string input = R"(
LABEL1 DS    0H
       USING LABEL2+*-LABEL1,1
LABEL2 DS    0H
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(using, drop_with_label)
{
    std::string input = R"(
LABEL DROP
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A251" }));
}

TEST(using, label_not_allowed_in_abs_space)
{
    std::string input = R"(
A     EQU    10
L     USING  A,1
      LA     0,L.A
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "ME004" }));
}

TEST(using, diag_complex_reloc)
{
    std::string input = R"(
TEST1 DSECT
TEST2 DSECT
A     EQU    TEST1+TEST2
      CSECT
L     USING  TEST1,1
      USING  TEST2,2
      LA     0,L.A
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "ME006", "ME009" }));
}

TEST(using, recursive_operand_inspection)
{
    std::string input = R"(
A     CSECT
L     USING  A,1
      DROP   ,
      LA     0,L.A-L.A
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "ME005", "ME005" }));
}

TEST(using, recursive_operand_inspection_dc)
{
    std::string input = R"(
A     CSECT
L     USING  A,1
      DROP   ,
      DC     A(L.A)
      DC     S(L.A)
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "ME005", "ME005", "ME007" }));
}

TEST(using, recursive_operand_inspection_literals_inactive)
{
    std::string input = R"(
A     CSECT
L     USING  A,1
      LARL   0,=A(L'=A(L.A))
      DROP   ,
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "ME005" }));
}

TEST(using, recursive_operand_inspection_literals_active)
{
    std::string input = R"(
A     CSECT
L     USING  A,1
      LARL   0,=A(L'=A(L.A))
      LTORG  ,
      DROP   ,
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(using, basic_operand_check)
{
    std::string input = R"(
A     CSECT
L     USING  A,1
      LA     0,L.A
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(using, basic_operand_check_dc)
{
    std::string input = R"(
A     CSECT
L     USING  A,1
      DC     A(L.A)
      DC     S(L.A)
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(using, out_of_range)
{
    std::string input = R"(
A     CSECT
L     USING  A,1
      LA     0,L.A+4096
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "ME008" }));
}

TEST(using, in_range_second_reg)
{
    std::string input = R"(
A     CSECT
L     USING  A,1,2
      LA     0,L.A+4096
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(using, disp_out_of_range)
{
    std::string input = R"(
A     CSECT
      USING  (A,A+8000),1
      LA     0,A+5000
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "ME008" }));
}

TEST(using, dispy_out_of_range)
{
    std::string input = R"(
A     CSECT
      USING  (A,A+8000),1
      LAY    0,A+1000000
      LLXAB  0,A+1000000
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "ME008", "ME008" }));
}

TEST(using, dispy_in_range)
{
    std::string input = R"(
A     CSECT
      USING  (A,A+8000),1
      LAY    0,A+10000
      LAY    0,A+5000
      LAY    0,A-5000
      LLXAB  0,A+10000
      LLXAB  0,A+5000
      LLXAB  0,A-5000
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(using, dc_s)
{
    std::string input = R"(
A     CSECT
      USING  A,1
      DC     S(0)
      DC     S(1(1))
      DC     S(A+100)
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(using, dc_s_out_of_range)
{
    std::string input = R"(
A     CSECT
      USING  (A,A+8192),1
      DC     S(A+5000)
      DC     S(A-5000)
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "ME008", "ME008" }));
}

TEST(using, dc_sy_in_range)
{
    std::string input = R"(
A     CSECT
      USING  (A,A+8192),1
      DC     SY(A+5000)
      DC     SY(A-5000)
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(using, dc_sy_out_of_range)
{
    std::string input = R"(
A     CSECT
      USING  (A,A+8192),1
      DC     SY(A+1000000)
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "ME008" }));
}

TEST(using, dc_s_invalid)
{
    std::string input = R"(
A     CSECT
      USING  (A,A+8192),1
      DC     S(A(1))
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "D034" }));
}

TEST(using, mnemonic_check_in_range)
{
    std::string input = R"(
A     CSECT
      USING  *,1
      B      *
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(using, mnemonic_check_missing_using)
{
    std::string input = R"(
A     CSECT
      B      *
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "ME007" }));
}

TEST(using, mnemonic_check_out_of_range)
{
    std::string input = R"(
A     CSECT
      USING  *,1
      B      *+4096
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "ME008" }));
}

TEST(using, trigger_lookahead_with_known_nominal)
{
    std::string input = R"(
    USING *,10
L   DS    F
    AIF   (L'A GT 0).T
.T  ANOP  ,
A   DC    S(L)
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(using, literal_with_known_nominal)
{
    std::string input = R"(
    USING *,10
L   DS    F
    LARL  0,=S(L)
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(using, simple_dc_with_known_nominal)
{
    std::string input = R"(
L   DS    F
    USING L,12
    DC    S(L)
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(using, long_displacement_instruction)
{
    std::string input = R"(
A   DS    A
    USING *,1
    CLIY  A,1
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(using, explicit_length)
{
    std::string input = R"(
    USING *,1
A   DS    A
B   DS    A
    MVC   A(2),B
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(using, tolerate_missing_sections)
{
    std::string input = R"(
    USING D,13
    USING MISSING,10
    ST    0,F

D   DSECT
F   DS    F
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "M113" }));
}
