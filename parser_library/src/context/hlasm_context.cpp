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

#include "hlasm_context.h"

#include <ctime>
#include <format>
#include <memory>
#include <numeric>
#include <ranges>

#include "context/id_storage.h"
#include "context/well_known.h"
#include "diagnostic_tools.h"
#include "ebcdic_encoding.h"
#include "expressions/evaluation_context.h"
#include "expressions/mach_expression.h"
#include "instructions/instruction.h"
#include "lexing/tools.h"
#include "ordinary_assembly/location_counter.h"
#include "using.h"
#include "utils/factory.h"
#include "utils/projectors.h"
#include "utils/time.h"
#include "variables/set_symbol.h"
#include "variables/system_variable.h"
#include "variables/variable.h"

namespace hlasm_plugin::parser_library::context {

code_scope* hlasm_context::curr_scope() { return &scope_stack_.back(); }

const code_scope* hlasm_context::curr_scope() const { return &scope_stack_.back(); }

void hlasm_context::init_instruction_map(opcode_map& opcodes, id_storage& ids, instruction_set_version active_instr_set)
{
    for (const auto& instr : instructions::all_machine_instructions())
    {
        if (!instruction_available(instr.instr_set_affiliation(), active_instr_set))
            continue;

        auto id = ids.add(instr.name());
        opcodes[id].emplace_back(opcode_t { id, &instr }, opcode_generation::zero);
    }
    for (const auto& instr : instructions::all_assembler_instructions())
    {
        auto id = ids.add(instr.name());
        opcodes[id].emplace_back(opcode_t { id, &instr }, opcode_generation::zero);
    }
    for (const auto& instr : instructions::all_ca_instructions())
    {
        auto id = ids.add(instr.name());
        opcodes[id].emplace_back(opcode_t { id, &instr }, opcode_generation::zero);
    }
    for (const auto& instr : instructions::all_mnemonic_codes())
    {
        if (!instruction_available(instr.instr_set_affiliation(), active_instr_set))
            continue;

        auto id = ids.add(instr.name());
        opcodes[id].emplace_back(opcode_t { id, &instr }, opcode_generation::zero);
    }
}

namespace {
class sysstmt_macro_param_data : public macro_param_data_single_dynamic
{
    const performance_metrics& metrics;
    const std::deque<code_scope>& scope_stack;

public:
    C_t get_dynamic_value() const override
    {
        size_t adjustment = 0;

        if (scope_stack.size() - 1 == 0)
        {
            // This is OPEN CODE -> add one
            adjustment = 1;
        }

        size_t sysstmt = metrics.macro_def_statements + metrics.macro_statements + metrics.open_code_statements
            + metrics.copy_statements + adjustment;

        return std::format("{:08}", sysstmt);
    };

    explicit sysstmt_macro_param_data(const performance_metrics& metrics, const std::deque<code_scope>& scope_stack)
        : macro_param_data_single_dynamic()
        , metrics(metrics)
        , scope_stack(scope_stack) {};
};

macro_data_ptr create_macro_data(std::string value)
{
    return std::make_unique<macro_param_data_single>(std::move(value));
}

template<typename T>
macro_data_ptr create_macro_data(std::unique_ptr<T> value)
{
    return value;
}

template<typename SYSTEM_VARIABLE_TYPE = system_variable, typename DATA>
std::pair<id_index, std::pair<std::shared_ptr<system_variable>, bool>> create_system_variable(
    id_index id, DATA mac_data, bool is_global)
{
    return {
        id,
        {
            std::make_shared<SYSTEM_VARIABLE_TYPE>(id, create_macro_data(std::move(mac_data))),
            is_global,
        },
    };
}

template<typename Func>
macro_data_ptr create_dynamic_var(Func f)
{
    struct dynamic_data final : public macro_param_data_single_dynamic
    {
        Func f;

        C_t get_dynamic_value() const override { return f(); }

        explicit dynamic_data(Func&& f)
            : f(std::move(f))
        {}
    };

    return std::make_unique<dynamic_data>(std::move(f));
}

auto time_sysvars(utils::timestamp now)
{
    struct
    {
        std::string datc_val;
        std::string date_val;
        std::string systime;
    } result = {
        std::format("{:04}{:02}{:02}", now.year(), now.month(), now.day()),
        std::format("{:02}/{:02}/{:02}", now.month(), now.day(), now.year() % 100),
        std::format("{:02}.{:02}", now.hour(), now.minute()),
    };
    return result;
}

} // namespace

std::vector<macro_data_ptr>& hlasm_context::ensure_dynamic_ptrs_count()
{
    auto& dp = dynamic_ptrs_vector;
    for (auto i = dp.size(); i < scope_stack_.size(); ++i)
    {
        if (i == 0)
            dp.emplace_back(create_dynamic_var([]() { return std::string("OPEN CODE"); }));
        else
            dp.emplace_back(create_dynamic_var([this, i]() {
                assert(i < scope_stack_.size());
                return scope_stack_[i].this_macro->id.to_string();
            }));
    }
    return dp;
}

system_variable_map hlasm_context::get_system_variables(const code_scope& scope)
{
    system_variable_map result;
    add_global_system_variables(result);
    auto match = [&scope](const auto& e) { return &scope == &e; };
    auto it = std::find_if(scope_stack_.rbegin(), scope_stack_.rend(), match);
    add_scoped_system_variables(result, it - scope_stack_.rbegin(), &scope == &scope_stack_.front());

    return result;
}

void hlasm_context::add_global_system_variables(system_variable_map& sysvars)
{
    // Available without macro scope
    auto [datc_val, date_val, systime] = time_sysvars(scope_stack_.front().time);

    sysvars.insert(create_system_variable(id_index("SYSDATC"), std::move(datc_val), true));
    sysvars.insert(create_system_variable(id_index("SYSDATE"), std::move(date_val), true));

    sysvars.insert(create_system_variable(id_index("SYSTIME"), std::move(systime), true));

    sysvars.insert(create_system_variable(id_index("SYSOPT_RENT"), std::to_string(asm_options_.sysopt_rent), true));

    sysvars.insert(
        create_system_variable(id_index("SYSOPT_XOBJECT"), std::to_string(asm_options_.sysopt_xobject), true));

    sysvars.insert(create_system_variable(id_index("SYSPARM"), asm_options_.sysparm, true));

    sysvars.insert(create_system_variable(
        id_index("SYSSTMT"), std::make_unique<sysstmt_macro_param_data>(metrics, scope_stack_), true));
    sysvars.insert(create_system_variable(id_index("SYSTEM_ID"), asm_options_.system_id, true));

    static constexpr auto emulated_hlasm_sysver = "1.6.0";
    sysvars.insert(create_system_variable(id_index("SYSVER"), emulated_hlasm_sysver, true));

    static constexpr auto emulated_asm_name = "HIGH LEVEL ASSEMBLER";
    sysvars.insert(create_system_variable(id_index("SYSASM"), emulated_asm_name, true));

    sysvars.insert(create_system_variable(
        id_index("SYSM_HSEV"), create_dynamic_var([this]() { return std::format("{:03}", mnote_max); }), true));
}

void hlasm_context::add_scoped_system_variables(system_variable_map& sysvars, size_t skip_last, bool globals_only)
{
    struct view_t
    {
        hlasm_context& ctx;
        size_t skip;

        auto size() const noexcept { return ctx.scope_stack_.size() - skip; }
        auto rbegin() const noexcept { return ctx.scope_stack_.rbegin() + skip; }
        auto rend() const noexcept { return ctx.scope_stack_.rend(); }
        const auto& top() const noexcept { return *rbegin(); }
        const auto& dyn_ptrs() const { return ctx.ensure_dynamic_ptrs_count(); }
    } view { *this, skip_last };

    sysvars.insert(create_system_variable(id_index("SYSM_SEV"),
        create_dynamic_var([view]() { return std::format("{:03}", view.top().mnote_last_max); }),
        true));

    if (globals_only)
        return;

    // Requires macro scope

    sysvars.insert(create_system_variable(id_index("SYSECT"),
        create_dynamic_var([view]() {
            if (auto l = view.top().loctr)
                return l->owner.name.to_string();
            return std::string();
        }),
        false));

    sysvars.insert(create_system_variable(
        id_index("SYSNDX"), create_dynamic_var([view]() { return std::format("{:04}", view.top().sysndx); }), false));

    sysvars.insert(create_system_variable(id_index("SYSSTYP"),
        create_dynamic_var([view]() -> std::string {
            if (!view.top().loctr)
                return "";
            switch (view.top().loctr->owner.kind)
            {
                case context::section_kind::COMMON:
                    return "COM";
                case context::section_kind::DUMMY:
                    return "DSECT";
                case context::section_kind::READONLY:
                    return "RSECT";
                case context::section_kind::EXECUTABLE:
                    return "CSECT";
                default:
                    return "";
            }
        }),
        false));

    sysvars.insert(create_system_variable(id_index("SYSLOC"),
        create_dynamic_var([view]() {
            if (view.top().loctr)
                return view.top().loctr->name.to_string();
            return std::string();
        }),
        false));

    sysvars.insert(create_system_variable(
        id_index("SYSNEST"), create_dynamic_var([view]() { return std::to_string(view.size() - 1); }), false));

    struct sysmac_data final : public macro_param_data_component
    {
        view_t view;

        explicit sysmac_data(view_t view)
            : macro_param_data_component(0)
            , view(view)
        {}

        A_t number() const override
        {
            assert(view.size() <= std::numeric_limits<A_t>::max());
            return (A_t)view.size();
        }

        // returns data of all nested classes in brackets separated by comma
        C_t get_value() const override
        {
            std::string result;

            result.append("(");
            for (auto it = view.rbegin(); it != std::prev(view.rend()); ++it)
            {
                result.append(it->this_macro->id.to_string()).push_back(',');
            }
            result.append("OPEN CODE)");

            return result;
        }

        // gets value of the idx-th value, when exceeds size of data, returns default value
        const macro_param_data_component* get_ith(A_t idx) const override
        {
            if (idx < 0 || idx >= view.size())
                return dummy.get();

            return view.dyn_ptrs()[view.size() - 1 - idx].get();
        }

        std::optional<std::pair<A_t, A_t>> index_range() const override
        {
            assert(view.size() <= std::numeric_limits<A_t>::max());
            return std::pair(0, (A_t)(view.size() - 1));
        }
    };

    sysvars.insert(
        create_system_variable<system_variable_sysmac>(id_index("SYSMAC"), std::make_unique<sysmac_data>(view), false));


    sysvars.insert(create_system_variable(id_index("SYSIN_DSN"), asm_options_.sysin_dsn, false));

    sysvars.insert(create_system_variable(id_index("SYSIN_MEMBER"), asm_options_.sysin_member, false));

    sysvars.insert(create_system_variable(
        id_index("SYSCLOCK"), create_dynamic_var([view]() { return view.top().time.to_string(); }), false));
}

hlasm_context::hlasm_context(
    utils::resource::resource_location file_loc, asm_option asm_options, std::shared_ptr<id_storage> init_ids)
    : ids_(std::move(init_ids))
    , opencode_file_location_(file_loc)
    , asm_options_(std::move(asm_options))
    , m_usings(std::make_unique<using_collection>())
    , m_active_usings(1, m_usings->remove_all())
    , m_statements_remaining(asm_options_.statement_count_limit)
    , ord_ctx(*this)
{
    scope_stack_.emplace_back().time = utils::timestamp::now().value_or(utils::timestamp(1900, 1, 1));

    init_instruction_map(opcode_mnemo_, *ids_, asm_options_.instr_set);

    add_global_system_variables(system_variables);
    add_scoped_system_variables(system_variables, 0, false);

    push_statement_processing(processing::processing_kind::ORDINARY, std::move(file_loc));
}

hlasm_context::~hlasm_context() = default;

void hlasm_context::set_source_position(position pos) { source_stack_.back().current_instruction.pos = pos; }

void hlasm_context::set_source_indices(size_t begin_index, size_t end_index)
{
    source_stack_.back().begin_index = begin_index;
    source_stack_.back().end_index = end_index;
}

std::pair<source_position, source_snapshot> hlasm_context::get_begin_snapshot(bool ignore_macros) const
{
    context::source_position statement_position;

    bool is_in_macros = ignore_macros ? false : scope_stack_.size() > 1;

    if (!is_in_macros && current_copy_stack().empty())
    {
        statement_position.rewind_target = current_source().begin_index;
    }
    else
    {
        statement_position.rewind_target = current_source().end_index;
    }

    context::source_snapshot snapshot = current_source().create_snapshot();

    if (!snapshot.copy_frames.empty() && is_in_macros)
        ++snapshot.copy_frames.back().statement_offset.value;

    return std::make_pair(std::move(statement_position), std::move(snapshot));
}

std::pair<source_position, source_snapshot> hlasm_context::get_end_snapshot() const
{
    context::source_position statement_position;
    statement_position.rewind_target = current_source().end_index;

    context::source_snapshot snapshot = current_source().create_snapshot();

    if (!snapshot.copy_frames.empty())
        ++snapshot.copy_frames.back().statement_offset.value;

    return std::make_pair(std::move(statement_position), std::move(snapshot));
}

void hlasm_context::push_statement_processing(const processing::processing_kind kind)
{
    assert(!source_stack_.empty());

    source_stack_.back().proc_stack.emplace_back(kind);
}

void hlasm_context::push_statement_processing(
    const processing::processing_kind kind, utils::resource::resource_location file_loc)
{
    source_stack_.emplace_back(std::move(file_loc), kind);
}

void hlasm_context::pop_statement_processing()
{
    assert(!source_stack_.empty());
    assert(!source_stack_.back().proc_stack.empty());

    source_stack_.back().proc_stack.pop_back();

    if (source_stack_.back().proc_stack.empty())
        source_stack_.pop_back();
}

id_index hlasm_context::add_id(std::string&& s) { return ids_->add(std::move(s)); }
id_index hlasm_context::add_id(std::string_view s) { return ids_->add(s); }
std::optional<id_index> hlasm_context::find_id(std::string_view s) const { return ids_->find(s); }

processing_stack_t hlasm_context::processing_stack()
{
    auto result = m_stack_tree.root();

    for (bool first = true; const auto& source : source_stack_)
    {
        result = m_stack_tree.step(result,
            source.current_instruction.pos,
            source.current_instruction.resource_loc,
            id_index(),
            file_processing_type::OPENCODE);
        for (const auto& member : source.copy_stack)
        {
            result = m_stack_tree.step(result,
                member.current_statement_position(),
                member.definition_location()->resource_loc,
                member.name(),
                file_processing_type::COPY);
        }

        if (first) // append macros immediately after ordinary processing
        {
            first = false;
            for (size_t j = 1; j < scope_stack_.size(); ++j)
            {
                for (auto type = file_processing_type::MACRO;
                     const auto& nest : scope_stack_[j].this_macro->get_current_copy_nest())
                {
                    result = m_stack_tree.step(result, nest.loc.pos, nest.loc.resource_loc, nest.member_name, type);
                    type = file_processing_type::COPY;
                }
            }
        }
    }

    return result;
}

processing_stack_details_t hlasm_context::processing_stack_details()
{
    std::vector<processing_frame_details> res;

    for (bool first = true; const auto& source : source_stack_)
    {
        res.emplace_back(source.current_instruction.pos,
            source.current_instruction.resource_loc,
            scope_stack_.front(),
            file_processing_type::OPENCODE,
            id_index());
        for (const auto& member : source.copy_stack)
        {
            res.emplace_back(member.current_statement_position(),
                member.definition_location()->resource_loc,
                scope_stack_.front(),
                file_processing_type::COPY,
                member.name());
        }

        if (first) // append macros immediately after ordinary processing
        {
            first = false;
            for (size_t j = 1; j < scope_stack_.size(); ++j)
            {
                for (auto type = file_processing_type::MACRO;
                     const auto& nest : scope_stack_[j].this_macro->get_current_copy_nest())
                {
                    res.emplace_back(nest.loc.pos, nest.loc.resource_loc, scope_stack_[j], type, nest.member_name);
                    type = file_processing_type::COPY;
                }
            }
        }
    }

    return res;
}

position hlasm_context::current_statement_position(bool consider_macros)
{
    if (consider_macros && source_stack_.size() == 1 && scope_stack_.size() > 1)
        return scope_stack_.back().this_macro->get_current_copy_nest().back().loc.pos;
    else if (!source_stack_.back().copy_stack.empty())
        return source_stack_.back().copy_stack.back().current_statement_position();
    else
        return source_stack_.back().current_instruction.pos;
}

location hlasm_context::current_statement_location(bool consider_macros)
{
    if (consider_macros && source_stack_.size() == 1 && scope_stack_.size() > 1)
        return scope_stack_.back().this_macro->get_current_copy_nest().back().loc;
    else if (!source_stack_.back().copy_stack.empty())
    {
        const auto& member = source_stack_.back().copy_stack.back();
        return location(member.current_statement_position(), member.definition_location()->resource_loc);
    }
    else
        return source_stack_.back().current_instruction;
}

const utils::resource::resource_location& hlasm_context::current_statement_source(bool consider_macros)
{
    if (consider_macros && source_stack_.size() == 1 && scope_stack_.size() > 1)
        return scope_stack_.back().this_macro->get_current_copy_nest().back().loc.resource_loc;
    else if (source_stack_.back().copy_stack.size())
        return source_stack_.back().copy_stack.back().definition_location()->resource_loc;
    else
        return source_stack_.back().current_instruction.resource_loc;
}

const std::deque<code_scope>& hlasm_context::scope_stack() const { return scope_stack_; }

const source_context& hlasm_context::current_source() const { return source_stack_.back(); }

const std::vector<copy_member_invocation>& hlasm_context::current_copy_stack() const
{
    return source_stack_.back().copy_stack;
}

std::vector<copy_member_invocation>& hlasm_context::current_copy_stack() { return source_stack_.back().copy_stack; }

const std::vector<copy_member_invocation>&
hlasm_plugin::parser_library::context::hlasm_context::opencode_copy_stack() const
{
    return source_stack_.front().copy_stack;
}

std::vector<copy_member_invocation>& hlasm_plugin::parser_library::context::hlasm_context::opencode_copy_stack()
{
    return source_stack_.front().copy_stack;
}

void hlasm_plugin::parser_library::context::hlasm_context::end_reached()
{
    assert(!m_end_reached);
    m_end_reached = true;

    scope_stack_.erase(scope_stack_.begin() + 1, scope_stack_.end());
    source_stack_.erase(source_stack_.begin() + 1, source_stack_.end());
    auto& opencode = source_stack_.front();
    opencode.copy_stack.clear();
    opencode.proc_stack.erase(opencode.proc_stack.begin() + 1, opencode.proc_stack.end());
}

std::vector<id_index> hlasm_context::whole_copy_stack() const
{
    std::vector<id_index> ret;

    for (auto& entry : source_stack_)
        for (auto& nest : entry.copy_stack)
            ret.push_back(nest.name());

    return ret;
}

const hlasm_context::global_variable_storage& hlasm_context::globals() const { return globals_; }

variable_symbol* hlasm_context::get_var_sym(
    id_index name, const code_scope& scope, const system_variable_map& sysvars) const
{
    if (auto tmp = scope.variables.find(name); tmp != scope.variables.end())
        return tmp->second.ref;

    if (auto s = sysvars.find(name); s && (scope.is_in_macro() || s->second.second))
        return s->second.first.get();

    if (scope.is_in_macro())
    {
        auto m_tmp = scope.this_macro->named_params.find(name);
        if (m_tmp != scope.this_macro->named_params.end())
            return m_tmp->second.get();
    }

    return nullptr;
}
variable_symbol* hlasm_context::get_var_sym(id_index name) const
{
    return get_var_sym(name, current_scope(), system_variables);
}

void hlasm_context::add_opencode_sequence_symbol(std::unique_ptr<opencode_sequence_symbol> seq_sym)
{
    opencode_sequence_symbols.try_emplace(seq_sym->name, std::move(seq_sym));
}

const sequence_symbol* hlasm_context::get_sequence_symbol(id_index name) const
{
    const auto* scope = curr_scope();
    label_storage::const_iterator found, end;

    if (scope->is_in_macro())
    {
        found = scope->this_macro->labels.find(name);
        end = scope->this_macro->labels.end();
    }
    else
    {
        found = opencode_sequence_symbols.find(name);
        end = opencode_sequence_symbols.end();
    }

    if (found != end)
        return found->second.get();
    else
        return nullptr;
}

const sequence_symbol* hlasm_context::get_opencode_sequence_symbol(id_index name) const
{
    if (const auto& sym = opencode_sequence_symbols.find(name); sym != opencode_sequence_symbols.end())
        return sym->second.get();
    return nullptr;
}

size_t hlasm_context::set_branch_counter(A_t value)
{
    curr_scope()->branch_counter = value;
    return ++curr_scope()->branch_counter_change;
}

int hlasm_context::get_branch_counter() const { return curr_scope()->branch_counter; }

void hlasm_context::decrement_branch_counter() { --curr_scope()->branch_counter; }

namespace {
enum class instruction_tag_type
{
    NONE,
    ASM,
    MAC,
};
constexpr std::string_view asm_tag = ":ASM";
constexpr std::string_view mac_tag = ":MAC";

constexpr std::string_view remove_instruction_tag(std::string_view s)
{
    static_assert(asm_tag.size() == mac_tag.size());
    s.remove_suffix(asm_tag.size());
    return s;
}

constexpr instruction_tag_type identify_tag(std::string_view s)
{
    using enum instruction_tag_type;

    const auto asm_ = s.ends_with(asm_tag);
    const auto mac_ = s.ends_with(mac_tag);
    if (asm_)
        return ASM;
    if (mac_)
        return MAC;
    return NONE;
}
} // namespace

template<typename Pred, typename Proj>
const opcode_t* hlasm_context::search_opcodes(id_index name, Pred p, Proj proj) const
{
    auto it = opcode_mnemo_.find(name);
    if (it == opcode_mnemo_.end())
        return nullptr;
    auto op = std::ranges::find_if(std::views::reverse(it->second), std::move(p), std::move(proj));
    if (op == it->second.rend())
        return nullptr;
    return &op->first;
}

const opcode_t* hlasm_context::search_opcodes(id_index name, opcode_generation gen) const
{
    return search_opcodes(name, [gen](const auto& e) { return e.second <= gen; });
}

const opcode_t* hlasm_context::find_opcode_mnemo(
    id_index name, opcode_generation gen, context::id_index* ext_suggestion) const
{
    switch (const auto name_view = name.to_string_view(); identify_tag(name_view))
    {
        case instruction_tag_type::NONE:
            if (ext_suggestion)
                *ext_suggestion = name;
            return search_opcodes(name, gen);

        case instruction_tag_type::ASM:
            if (const auto without_tag = ids_->find(remove_instruction_tag(name_view)))
            {
                if (const auto* op = search_opcodes(*without_tag, gen); op && op->is_asm())
                    return op;
            }
            break;

        case instruction_tag_type::MAC: {
            if (const auto* tagged_macro = search_opcodes(name, gen); tagged_macro && tagged_macro->is_macro())
                return tagged_macro;

            const auto without_tag = remove_instruction_tag(name_view);
            if (identify_tag(without_tag) != instruction_tag_type::NONE)
                return nullptr;

            const auto id = ids_->add(without_tag);
            if (ext_suggestion)
                *ext_suggestion = id;
            if (const auto* op = search_opcodes(id, gen); op && op->is_macro())
                return op;
            break;
        }
    }

    return nullptr;
}

const opcode_t* hlasm_context::find_any_valid_opcode(id_index name) const
{
    switch (const auto name_view = name.to_string_view(); identify_tag(name_view))
    {
        case instruction_tag_type::NONE:
            return search_opcodes(name, [](const opcode_t& op) { return !op.empty(); }, utils::first_element);

        case instruction_tag_type::ASM:
            if (const auto without_tag = ids_->find(remove_instruction_tag(name_view)))
                return search_opcodes(*without_tag, &opcode_t::is_asm, utils::first_element);
            break;

        case instruction_tag_type::MAC: {
            if (const auto* tagged = search_opcodes(name, &opcode_t::is_macro, utils::first_element))
                return tagged;

            const auto without_tag = remove_instruction_tag(name_view);
            if (identify_tag(without_tag) != instruction_tag_type::NONE)
                return nullptr;

            if (const auto id = ids_->find(without_tag))
                return search_opcodes(*id, &opcode_t::is_macro, utils::first_element);

            break;
        }
    }

    return nullptr;
}

bool hlasm_context::add_mnemonic(id_index mnemo, id_index op_code)
{
    const auto* op = find_opcode_mnemo(op_code);
    if (!op || op->empty())
        return false;

    opcode_mnemo_[mnemo].emplace_back(*op, ++m_current_opcode_generation);

    return true;
}

bool hlasm_context::remove_mnemonic(id_index mnemo)
{
    if (const auto* op = find_opcode_mnemo(mnemo); !op || op->empty())
        return false;

    if (auto it = external_macros_.find(mnemo); it == external_macros_.end())
        opcode_mnemo_[mnemo].emplace_back(opcode_t(), ++m_current_opcode_generation);
    else // restore external macro when available
        opcode_mnemo_[mnemo].emplace_back(opcode_t { it->first, it->second.get() }, ++m_current_opcode_generation);

    return true;
}

opcode_t hlasm_context::get_operation_code(id_index symbol, context::id_index* ext_suggestion) const
{
    if (auto it = find_opcode_mnemo(symbol, opcode_generation::current, ext_suggestion); it)
        return *it;
    else
        return opcode_t();
}

SET_t hlasm_context::get_attribute_value_ord(data_attr_kind attribute, id_index symbol_name)
{
    if (attribute == data_attr_kind::O)
        return get_opcode_attr(symbol_name);
    return get_attribute_value_ord(attribute, ord_ctx.get_symbol(symbol_name));
}

SET_t hlasm_context::get_attribute_value_ord(data_attr_kind attribute, const symbol* symbol)
{
    switch (attribute)
    {
        case data_attr_kind::D:
            if (symbol)
                return 1;
            return 0;
        case data_attr_kind::T:
            if (symbol)
            {
                auto attr_val = symbol->attributes().get_attribute_value(attribute);
                return ebcdic_encoding::to_ascii((unsigned char)attr_val);
            }
            return "U";
        case data_attr_kind::O:
            if (symbol)
                return get_opcode_attr(symbol->name());
            return "U";
        default:
            if (symbol)
                return symbol->attributes().get_attribute_value(attribute);
            return symbol_attributes::default_value(attribute);
    }
}

struct opcode_attr_visitor
{
    std::string operator()(const instructions::assembler_instruction*) const { return "A"; }
    std::string operator()(const instructions::ca_instruction*) const { return "A"; }
    std::string operator()(const instructions::mnemonic_code*) const { return "E"; }
    std::string operator()(const instructions::machine_instruction*) const { return "O"; }
    std::string operator()(macro_definition*) const { return "M"; }
    std::string operator()(std::monostate) const { return "U"; }
};

C_t hlasm_context::get_opcode_attr(id_index symbol, opcode_generation gen) const
{
    auto op = find_opcode_mnemo(symbol, gen);
    if (!op)
        return "U";

    return std::visit(opcode_attr_visitor(), op->opcode_detail);
}

macro_def_ptr hlasm_context::add_macro(id_index name,
    id_index label_param_name,
    std::vector<macro_arg> params,
    statement_block definition,
    copy_nest_storage copy_nests,
    label_storage labels,
    location definition_location,
    std::unordered_set<copy_member_ptr> used_copy_members,
    bool external)
{
    auto result = std::make_shared<macro_definition>(name,
        label_param_name,
        std::move(params),
        std::move(definition),
        std::move(copy_nests),
        std::move(labels),
        std::move(definition_location),
        std::move(used_copy_members));
    add_macro(result, external);
    return result;
}

void hlasm_context::add_macro(macro_def_ptr macro, bool external)
{
    auto next_gen = ++m_current_opcode_generation;
    const auto& m = macros_[macro->id].emplace_back(std::move(macro), next_gen).first;
    auto& mnemo = opcode_mnemo_[m->id];
    mnemo.emplace_back(opcode_t { m->id, m.get() }, next_gen);
    if (external)
        external_macros_.try_emplace(m->id, m);
    // hide strange MAC:ASM named macros
    if (identify_tag(m->id.to_string_view()) == instruction_tag_type::ASM)
        mnemo.emplace_back(opcode_t(), ++m_current_opcode_generation);
};

const hlasm_context::macro_storage& hlasm_context::macros() const { return macros_; }

const macro_def_ptr* hlasm_context::find_macro(id_index name, opcode_generation gen) const
{
    auto it = macros_.find(name);
    if (it == macros_.end())
        return nullptr;
    auto mit = std::find_if(it->second.rbegin(), it->second.rend(), [gen](const auto& e) { return e.second <= gen; });
    if (mit == it->second.rend())
        return nullptr;
    return &mit->first;
}

const hlasm_context::opcode_map& hlasm_context::opcode_mnemo_storage() const { return opcode_mnemo_; }

context::macro_definition* hlasm_context::get_macro_definition(id_index name, context::opcode_generation gen) const
{
    if (auto mnem = find_opcode_mnemo(name, gen); mnem && mnem->is_macro())
        return mnem->get_macro_details();

    return nullptr;
}

bool hlasm_context::is_in_macro() const { return scope_stack_.back().is_in_macro(); }

std::pair<const macro_invocation*, bool> hlasm_context::enter_macro(
    macro_definition* macro_def, macro_data_ptr label_param_data, std::vector<macro_arg> params)
{
    assert(SYSNDX_ <= SYSNDX_limit);

    if (label_param_data)
    {
        if (auto label = label_param_data->get_value(); lexing::is_valid_symbol_name(label))
            ord_ctx.symbol_mentioned_on_macro(add_id(std::move(label)));
    }

    auto [invo, truncated] = macro_def->call(std::move(label_param_data), std::move(params), well_known::SYSLIST);
    auto* const result = invo.get();

    auto& new_scope = scope_stack_.emplace_back(std::move(invo));
    new_scope.time = utils::timestamp::now().value_or(utils::timestamp(1900, 1, 1));
    new_scope.sysndx = SYSNDX_;
    if (auto sect = ord_ctx.current_section(); sect)
        new_scope.loctr = &sect->current_location_counter();

    ++SYSNDX_;

    return { result, truncated };
}

void hlasm_context::leave_macro()
{
    auto mnote_last_max = scope_stack_.back().mnote_max_in_scope;
    scope_stack_.pop_back();
    scope_stack_.back().mnote_last_max = mnote_last_max;
}

const macro_invocation* hlasm_context::current_macro() const
{
    if (is_in_macro())
        return curr_scope()->this_macro.get();
    return nullptr;
}

const location* hlasm_context::current_macro_definition_location() const
{
    if (is_in_macro())
    {
        if (const auto& mac = curr_scope()->this_macro; mac)
            return &mac->definition_location;
    }
    return nullptr;
}

const utils::resource::resource_location& hlasm_context::opencode_location() const { return opencode_file_location_; }

copy_member_ptr hlasm_context::add_copy_member(
    id_index member, statement_block definition, location definition_location)
{
    auto [it, _] = copy_members_.try_emplace(member, utils::factory([&]() {
        return std::make_shared<copy_member>(member, std::move(definition), std::move(definition_location));
    }));

    return it->second;
}

void hlasm_context::add_copy_member(copy_member_ptr member) { copy_members_[member->name] = std::move(member); }


copy_member_ptr hlasm_context::get_copy_member(id_index member) const
{
    if (auto it = copy_members_.find(member); it != copy_members_.end())
        return it->second;
    return nullptr;
}

void hlasm_context::enter_copy_member(id_index member_name)
{
    auto tmp = copy_members_.find(member_name);
    assert(tmp != copy_members_.end());

    const auto& [name, member] = *tmp;

    source_stack_.back().copy_stack.emplace_back(copy_member_invocation(member));
}

const hlasm_context::copy_member_storage& hlasm_context::copy_members() { return copy_members_; }

void hlasm_context::leave_copy_member() { source_stack_.back().copy_stack.pop_back(); }

template<typename T>
set_symbol_base* hlasm_context::create_global_variable(id_index id, bool is_scalar)
{
    auto* scope = curr_scope();

    if (auto var = scope->variables.find(id); var != scope->variables.end())
        return var->second.ref->template access_set_symbol<T>();

    auto [it, _] = globals_.try_emplace(id, std::in_place_type<set_symbol<T>>, id, is_scalar);

    if (!std::holds_alternative<set_symbol<T>>(it->second))
        return nullptr;

    scope->variables.try_emplace(id, &std::get<set_symbol<T>>(it->second), true);

    return &std::get<set_symbol<T>>(it->second);
}

template set_symbol_base* hlasm_context::create_global_variable<A_t>(id_index id, bool is_scalar);
template set_symbol_base* hlasm_context::create_global_variable<B_t>(id_index id, bool is_scalar);
template set_symbol_base* hlasm_context::create_global_variable<C_t>(id_index id, bool is_scalar);

template<typename T>
set_symbol_base* hlasm_context::create_local_variable(id_index id, bool is_scalar)
{
    return curr_scope()
        ->variables.try_emplace(id, std::in_place_type<T>, id, is_scalar, false)
        .first->second.ref->template access_set_symbol<T>();
}

template set_symbol_base* hlasm_context::create_local_variable<A_t>(id_index id, bool is_scalar);
template set_symbol_base* hlasm_context::create_local_variable<B_t>(id_index id, bool is_scalar);
template set_symbol_base* hlasm_context::create_local_variable<C_t>(id_index id, bool is_scalar);

void hlasm_context::apply_source_snapshot(source_snapshot snapshot)
{
    assert(std::transform_reduce(source_stack_.begin(),
               source_stack_.end(),
               (size_t)0,
               std::plus {},
               [](const auto& source) { return source.proc_stack.size(); })
        == 1);

    source_stack_.back().current_instruction = std::move(snapshot.instruction);
    source_stack_.back().begin_index = snapshot.begin_index;
    source_stack_.back().end_index = snapshot.end_index;

    source_stack_.back().copy_stack.clear();

    for (auto& frame : snapshot.copy_frames)
    {
        copy_member_invocation invo(copy_members_.at(frame.copy_member));
        invo.current_statement = frame.statement_offset;
        source_stack_.back().copy_stack.push_back(std::move(invo));
    }
}

const code_scope& hlasm_context::current_scope() const { return *curr_scope(); }

void hlasm_context::update_mnote_max(unsigned mnote_level)
{
    mnote_max = std::max(mnote_max, mnote_level);
    scope_stack_.back().mnote_max_in_scope = std::max(scope_stack_.back().mnote_max_in_scope, mnote_level);
}

void hlasm_context::using_add(id_index label,
    std::unique_ptr<expressions::mach_expression> begin,
    std::unique_ptr<expressions::mach_expression> end,
    std::vector<std::unique_ptr<expressions::mach_expression>> bases,
    dependency_evaluation_context eval_ctx,
    processing_stack_t stack)
{
    m_active_usings.back() = m_usings->add(m_active_usings.back(),
        label,
        std::move(begin),
        std::move(end),
        std::move(bases),
        std::move(eval_ctx),
        std::move(stack));
}

void hlasm_context::using_remove(std::vector<std::unique_ptr<expressions::mach_expression>> bases,
    dependency_evaluation_context eval_ctx,
    processing_stack_t stack)
{
    m_active_usings.back() = bases.empty()
        ? m_usings->remove_all()
        : m_usings->remove(m_active_usings.back(), std::move(bases), std::move(eval_ctx), std::move(stack));
}

void hlasm_context::using_push() { m_active_usings.push_back(m_active_usings.back()); }

bool hlasm_context::using_pop()
{
    if (m_active_usings.size() == 1)
        return false;

    m_active_usings.pop_back();
    return true;
}

void hlasm_context::using_resolve(diagnostic_consumer& diag, const library_info& li)
{
    m_usings->resolve_all(ord_ctx, diag, li);
}

index_t<using_collection> hlasm_context::using_current() const { return m_active_usings.back(); }

hlasm_context::name_result hlasm_context::try_get_symbol_name(std::string_view symbol)
{
    if (!lexing::is_ord_symbol(symbol))
        return std::make_pair(false, context::id_index());

    return std::make_pair(true, add_id(symbol));
}

hlasm_context::name_result hlasm_context::try_get_symbol_name(id_index symbol) const
{
    if (!lexing::is_ord_symbol(symbol.to_string_view()))
        return std::make_pair(false, context::id_index());

    return std::make_pair(true, symbol);
}

bool hlasm_context::register_psect(id_index symbol, id_index psect)
{
    auto [_, inserted] = psect_registrations.try_emplace(symbol, psect, processing_stack());
    return inserted;
}

namespace {
const section* extract_symbol_base(const symbol* s)
{
    if (s->kind() != symbol_value_kind::RELOC)
        return nullptr;
    const auto& bases = s->value().get_reloc().bases();
    if (bases.size() != 1)
        return nullptr;
    const auto& [base, mult] = bases.front();
    if (mult != 1)
        return nullptr;
    return base.owner;
}

bool psect_compatible_symbol(const symbol* s)
{
    auto attr = s->attributes().origin();
    return attr != symbol_origin::EQU && attr != symbol_origin::UNKNOWN;
}

bool psect_compatible_section(const section* s)
{
    return s->kind == section_kind::EXECUTABLE || s->kind == section_kind::READONLY;
}
} // namespace

void hlasm_context::validate_psect_registrations(diagnostic_consumer& diags)
{
    for (const auto& [sym, details] : psect_registrations)
    {
        const auto& [psect, stack] = details;
        const auto* section = ord_ctx.get_section(psect);
        if (const symbol * s; !section && (s = ord_ctx.get_symbol(psect)) != nullptr
            && (!psect_compatible_symbol(s) || (section = extract_symbol_base(s)) == nullptr))
        {
            diags.add_diagnostic(
                add_stack_details(diagnostic_op::error_A173_invalid_psect(range(stack.frame().pos)), stack));
            continue;
        }
        if (!section)
        {
            diags.add_diagnostic(
                add_stack_details(diagnostic_op::error_E032(psect.to_string_view(), range(stack.frame().pos)), stack));
            continue;
        }
        if (!psect_compatible_section(section))
        {
            diags.add_diagnostic(
                add_stack_details(diagnostic_op::error_A173_invalid_psect(range(stack.frame().pos)), stack));
            continue;
        }
    }
}

const code_scope& get_current_scope(const context::hlasm_context& ctx) { return ctx.current_scope(); }
variable_symbol* get_var_sym(const expressions::evaluation_context& eval_ctx, id_index name)
{
    return eval_ctx.hlasm_ctx.get_var_sym(
        name, eval_ctx.active_scope(), eval_ctx.sysvars ? *eval_ctx.sysvars : eval_ctx.hlasm_ctx.system_variables);
}

SET_t get_var_sym_value(const expressions::evaluation_context& eval_ctx,
    id_index name,
    std::span<const context::A_t> subscript,
    range symbol_range)
{
    auto var = get_var_sym(eval_ctx, name);
    if (!test_symbol_for_read(var, subscript, symbol_range, eval_ctx.diags, name.to_string_view()))
        return SET_t();

    if (auto set_sym = var->access_set_symbol_base())
    {
        if (subscript.empty())
        {
            switch (set_sym->type)
            {
                case SET_t_enum::A_TYPE:
                    return set_sym->access_set_symbol<A_t>()->get_value();
                case SET_t_enum::B_TYPE:
                    return set_sym->access_set_symbol<B_t>()->get_value();
                case SET_t_enum::C_TYPE:
                    return set_sym->access_set_symbol<C_t>()->get_value();
                default:
                    return SET_t();
            }
        }
        else
        {
            const auto idx = subscript.front();

            switch (set_sym->type)
            {
                case SET_t_enum::A_TYPE:
                    return set_sym->access_set_symbol<A_t>()->get_value(idx);
                case SET_t_enum::B_TYPE:
                    return set_sym->access_set_symbol<B_t>()->get_value(idx);
                case SET_t_enum::C_TYPE:
                    return set_sym->access_set_symbol<C_t>()->get_value(idx);
                default:
                    return SET_t();
            }
        }
    }
    else if (auto mac_par = var->access_macro_param_base())
    {
        return mac_par->get_value(subscript);
    }
    return SET_t();
}

bool test_symbol_for_read(const variable_symbol* var,
    std::span<const context::A_t> subscript,
    range symbol_range,
    diagnostic_op_consumer& diags,
    std::string_view var_name)
{
    if (!var)
    {
        diags.add_diagnostic(
            diagnostic_op::error_E010("variable", var_name, symbol_range)); // error - unknown name of variable
        return false;
    }

    return var->can_read(subscript, symbol_range, diags);
}

std::shared_ptr<id_storage> hlasm_context::make_default_id_storage() { return std::make_shared<id_storage>(); }

} // namespace hlasm_plugin::parser_library::context
