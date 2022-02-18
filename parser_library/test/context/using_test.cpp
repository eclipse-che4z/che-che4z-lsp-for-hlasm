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
    hlasm_context hlasm_ctx;
    ordinary_assembly_context asm_ctx;
    test_context()
        : asm_ctx(hlasm_ctx.ids(), hlasm_ctx)
    {}

    std::map<std::string, section> m_sect;
    std::map<id_index, symbol> m_symbols;
    std::optional<address> m_loctr;

    id_index id(const std::string& s) { return hlasm_ctx.ids().add(s); }
    id_index label(const std::string& s)
    {
        auto label = hlasm_ctx.ids().add(s);
        asm_ctx.register_using_label(label);
        return label;
    }
    const section* create_section(const std::string& s)
    {
        asm_ctx.set_section(id(s), section_kind::COMMON, location());
        return asm_ctx.current_section();
    }

    address addr(const std::string& name, const std::string& sect, int offset)
    {
        address result({ create_section(sect) }, offset, {});
        m_symbols.try_emplace(
            id(name), id(name), result, symbol_attributes(symbol_origin::EQU), location(), processing_stack_t());
        return result;
    }

    std::unique_ptr<mach_expression> symbol(const std::string& n, const std::string& q = "")
    {
        return std::make_unique<mach_expr_symbol>(id(n), q.empty() ? nullptr : id(q), range());
    }

    std::unique_ptr<mach_expression> number(int n) const { return std::make_unique<mach_expr_constant>(n, range()); }

    std::unique_ptr<mach_expression> loctr() const { return std::make_unique<mach_expr_location_counter>(range()); }

    // Inherited via dependency_solver
    virtual const context::symbol* get_symbol(id_index name) const override { return &m_symbols.at(name); }
    virtual std::optional<address> get_loctr() const override { return m_loctr; }
    virtual id_index get_literal_id(
        const std::string&, const std::shared_ptr<const expressions::data_definition>&, const range&, bool) override
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
    diagnostic_consumer_container<diagnostic_s> d_s;

    auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect = coll.add(current, nullptr, c.symbol("SECT"), nullptr, args(c.number(1)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(d_s.diags.empty());

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
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect =
        coll.add(current, nullptr, c.symbol("SECT"), nullptr, args(c.number(2), c.number(1)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(d_s.diags.empty());

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
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect =
        coll.add(current, nullptr, c.symbol("SECT") + c.number(10), nullptr, args(c.number(2)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(d_s.diags.empty());

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
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect =
        coll.add(current, nullptr, c.symbol("SECT") - c.number(10), nullptr, args(c.number(2)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(d_s.diags.empty());

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
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    auto sect = c.create_section("SECT");
    auto sect2 = c.create_section("SECT2");
    /*
     * USING SECT+10,12
     * USING SECT2+5,SECT+20
     */

    [[maybe_unused]] auto with_sect =
        coll.add(current, nullptr, c.symbol("SECT") + c.number(10), nullptr, args(c.number(12)), {}, {});
    [[maybe_unused]] auto with_sect2 = coll.add(
        with_sect, nullptr, c.symbol("SECT2") + c.number(5), nullptr, args(c.symbol("SECT") + c.number(20)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(d_s.diags.empty());

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
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    auto sect = c.create_section("SECT");
    auto label = c.id("LABEL");

    [[maybe_unused]] auto with_sect = coll.add(current, label, c.symbol("SECT"), nullptr, args(c.number(1)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(d_s.diags.empty());

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
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect =
        coll.add(current, nullptr, c.symbol("SECT"), nullptr, args(c.number(2), c.number(1)), {}, {});

    [[maybe_unused]] auto after_drop2 = coll.remove(with_sect, args(c.number(2)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(d_s.diags.empty());

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
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    auto sect = c.create_section("SECT");
    auto sect2 = c.create_section("SECT2");

    [[maybe_unused]] auto with_sect = coll.add(current, nullptr, c.symbol("SECT"), nullptr, args(c.number(2)), {}, {});
    [[maybe_unused]] auto with_sect2 =
        coll.add(with_sect, nullptr, c.symbol("SECT2"), nullptr, args(c.symbol("SECT")), {}, {});

    [[maybe_unused]] auto after_drop2 = coll.remove(with_sect2, args(c.number(2)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(d_s.diags.empty());

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
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    auto sect = c.create_section("SECT");
    auto sect2 = c.create_section("SECT2");
    auto label = c.id("LABEL");

    [[maybe_unused]] auto with_sect = coll.add(current, label, c.symbol("SECT"), nullptr, args(c.number(1)), {}, {});
    [[maybe_unused]] auto with_sect2 =
        coll.add(with_sect, label, c.symbol("SECT2"), nullptr, args(c.number(1)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(d_s.diags.empty());

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
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    auto label = c.id("LABEL");
    auto sect = c.create_section("SECT");
    auto sect2 = c.create_section("SECT2");

    [[maybe_unused]] auto with_sect = coll.add(current, nullptr, c.symbol("SECT"), nullptr, args(c.number(2)), {}, {});
    [[maybe_unused]] auto with_sect2 =
        coll.add(with_sect, label, c.symbol("SECT2"), nullptr, args(c.symbol("SECT")), {}, {});

    [[maybe_unused]] auto after_drop2 = coll.remove(with_sect2, args(c.number(2)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(d_s.diags.empty());

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

TEST(using, no_drop_warning)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    [[maybe_unused]] auto after_drop2 = coll.remove(current, args(c.number(2)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U001" }));
}

TEST(using, use_reg_16)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    [[maybe_unused]] auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect = coll.add(current, nullptr, c.symbol("SECT"), nullptr, args(c.number(16)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "M120" }));
}

TEST(using, use_non_simple_reloc)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    [[maybe_unused]] auto with_sect =
        coll.add(current, nullptr, c.symbol("SECT"), nullptr, args(c.symbol("LABEL") + c.symbol("LABEL")), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "M113" }));
}

TEST(using, drop_qualified_label)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    [[maybe_unused]] auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect =
        coll.add(current, c.label("LABEL"), c.symbol("SECT"), nullptr, args(c.number(1)), {}, {});

    [[maybe_unused]] auto after_drop2 = coll.remove(with_sect, args(c.symbol("LABEL", "LABEL")), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U003" }));
}

TEST(using, drop_reg_16)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    [[maybe_unused]] auto after_drop2 = coll.remove(current, args(c.number(16)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U003" }));
}

TEST(using, drop_reloc)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    [[maybe_unused]] auto sect = c.create_section("LABEL");
    [[maybe_unused]] auto after_drop2 = coll.remove(current, args(c.symbol("LABEL") + c.symbol("LABEL")), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U003" }));
}

TEST(using, dependent_no_active_using)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    [[maybe_unused]] auto sect = c.create_section("SECT");
    [[maybe_unused]] auto sect2 = c.create_section("SECT2");

    [[maybe_unused]] auto with_sect2 = coll.add(
        current, nullptr, c.symbol("SECT2") + c.number(5), nullptr, args(c.symbol("SECT") + c.number(20)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U004" }));
}

TEST(using, dependent_no_active_matching_using)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    [[maybe_unused]] auto sect = c.create_section("SECT");
    [[maybe_unused]] auto sect2 = c.create_section("SECT2");
    [[maybe_unused]] auto sect3 = c.create_section("SECT3");
    /*
     * USING SECT+10,12
     * USING SECT2+5,SECT+20
     */

    [[maybe_unused]] auto with_sect =
        coll.add(current, nullptr, c.symbol("SECT") + c.number(10), nullptr, args(c.number(12)), {}, {});
    [[maybe_unused]] auto with_sect2 = coll.add(
        with_sect, nullptr, c.symbol("SECT2") + c.number(5), nullptr, args(c.symbol("SECT3") + c.number(20)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U004" }));
}

TEST(using, using_undefined_begin)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    [[maybe_unused]] auto with_sect = coll.add(current, nullptr, c.symbol("SECT"), nullptr, args(c.number(1)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "M113" }));
}

TEST(using, using_qualified_begin)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    [[maybe_unused]] auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect =
        coll.add(current, nullptr, c.symbol("SECT", "LABEL"), nullptr, args(c.number(1)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U002" }));
}

TEST(using, using_undefined_end)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    [[maybe_unused]] auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect =
        coll.add(current, nullptr, c.symbol("SECT"), c.symbol("SECT_END"), args(c.number(1)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "M113" }));
}

TEST(using, using_qualified_end)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    [[maybe_unused]] auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect = coll.add(
        current, nullptr, c.symbol("SECT"), c.symbol("SECT", "LABEL") + c.number(1), args(c.number(1)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U002" }));
}

TEST(using, using_bad_range)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    [[maybe_unused]] auto sect = c.create_section("SECT");
    [[maybe_unused]] auto sect2 = c.create_section("SECT2");

    [[maybe_unused]] auto with_sect =
        coll.add(current, nullptr, c.symbol("SECT"), c.symbol("SECT"), args(c.number(1)), {}, {});
    [[maybe_unused]] auto with_sect2 =
        coll.add(current, nullptr, c.symbol("SECT"), c.symbol("SECT2"), args(c.number(1)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U005", "U005" }));
}


TEST(using, use_complex_reloc)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    [[maybe_unused]] auto s1 = c.create_section("S1");
    [[maybe_unused]] auto s2 = c.create_section("S2");
    [[maybe_unused]] auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect =
        coll.add(current, nullptr, c.symbol("SECT"), nullptr, args(c.symbol("S1") + c.symbol("S2")), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "M120" }));
}

TEST(using, drop_invalid)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    [[maybe_unused]] auto after_drop2 = coll.remove(current, args(c.symbol("LABEL")), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U003" }));
}

TEST(using, drop_inactive)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    [[maybe_unused]] auto label = c.label("LABEL");

    [[maybe_unused]] auto after_drop2 = coll.remove(current, args(c.symbol("LABEL")), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U001" }));
}

TEST(using, basic_with_limit)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect =
        coll.add(current, nullptr, c.symbol("SECT"), c.symbol("SECT") + c.number(10), args(c.number(1)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(d_s.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 0, false), evaluate_result(1, 0));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 9, false), evaluate_result(1, 9));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, sect, 20, false), evaluate_result(invalid_register, 11));
}

TEST(using, duplicate_base)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    [[maybe_unused]] auto sect = c.create_section("SECT");

    [[maybe_unused]] auto with_sect =
        coll.add(current, nullptr, c.symbol("SECT"), nullptr, args(c.number(1), c.number(1)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(matches_message_codes(d_s.diags, { "U006" }));
}

TEST(using, absolute)
{
    test_context c;

    using_collection coll;
    index_t<using_collection> current;
    diagnostic_consumer_container<diagnostic_s> d_s;

    [[maybe_unused]] auto with_sect = coll.add(current, nullptr, c.number(128), nullptr, args(c.number(1)), {}, {});

    coll.resolve_all(c.asm_ctx, d_s);

    EXPECT_TRUE(d_s.diags.empty());

    EXPECT_EQ(coll.evaluate(with_sect, nullptr, nullptr, 0, false), evaluate_result(0, 0));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, nullptr, 10, false), evaluate_result(0, 10));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, nullptr, 128, false), evaluate_result(1, 0));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, nullptr, 256, false), evaluate_result(1, 128));
    EXPECT_EQ(coll.evaluate(with_sect, nullptr, nullptr, -100, true), evaluate_result(0, -100));
}

TEST(using, simple_using)
{
    std::string input = R"(
TEST  CSECT
LABEL USING TEST,1
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

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
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A164", "A104" }));
}

TEST(using, wrong_operands)
{
    std::string input = R"(
LABEL USING 1,(1,1)
)";

    analyzer a(input);
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A164" }));
}
