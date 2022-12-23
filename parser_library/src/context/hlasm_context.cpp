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
#include <memory>
#include <numeric>

#include "ebcdic_encoding.h"
#include "expressions/conditional_assembly/terms/ca_constant.h"
#include "expressions/conditional_assembly/terms/ca_symbol_attribute.h"
#include "instruction.h"
#include "lexing/lexer.h"
#include "lexing/tools.h"
#include "using.h"
#include "utils/time.h"

namespace hlasm_plugin::parser_library::context {

code_scope* hlasm_context::curr_scope() { return &scope_stack_.back(); }

const code_scope* hlasm_context::curr_scope() const { return &scope_stack_.back(); }

void hlasm_context::init_instruction_map(opcode_map& opcodes, id_storage& ids, instruction_set_version active_instr_set)
{
    for (const auto& instr : instruction::all_machine_instructions())
    {
        if (!instruction_available(instr.instr_set_affiliation(), active_instr_set))
            continue;

        auto id = ids.add(instr.name());
        opcodes.try_emplace({ id, opcode_generation::zero }, opcode_t { id, &instr });
    }
    for (const auto& instr : instruction::all_assembler_instructions())
    {
        auto id = ids.add(instr.name());
        opcodes.try_emplace({ id, opcode_generation::zero }, opcode_t { id, &instr });
    }
    for (const auto& instr : instruction::all_ca_instructions())
    {
        auto id = ids.add(instr.name());
        opcodes.try_emplace({ id, opcode_generation::zero }, opcode_t { id, &instr });
    }
    for (const auto& instr : instruction::all_mnemonic_codes())
    {
        if (!instruction_available(instr.instr_set_affiliation(), active_instr_set))
            continue;

        auto id = ids.add(instr.name());
        opcodes.try_emplace({ id, opcode_generation::zero }, opcode_t { id, &instr });
    }
}

namespace {
std::string left_pad(std::string s, size_t len, char c = ' ')
{
    if (auto string_len = s.length(); string_len < len)
        s.insert(0, len - string_len, c);

    return s;
}

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

        return left_pad(std::to_string(sysstmt), 8, '0');
    };

    explicit sysstmt_macro_param_data(const performance_metrics& metrics, const std::deque<code_scope>& scope_stack)
        : macro_param_data_single_dynamic()
        , metrics(metrics)
        , scope_stack(scope_stack) {};
};

template</*std::integral*/ typename T, size_t padding>
class integral_macro_param_data : public macro_param_data_single_dynamic
{
    const T& m_value;

public:
    C_t get_dynamic_value() const override { return left_pad(std::to_string(m_value), padding, '0'); };

    explicit integral_macro_param_data(const T& value)
        : m_value(value) {};
};

macro_data_ptr create_macro_data(std::string value)
{
    return std::make_unique<macro_param_data_single>(std::move(value));
}

macro_data_ptr create_macro_data(std::vector<std::string> value)
{
    std::vector<macro_data_ptr> data;
    for (auto& single_data : value)
    {
        data.push_back(std::make_unique<macro_param_data_single>(std::move(single_data)));
    }

    return std::make_unique<macro_param_data_composite>(std::move(data));
}

macro_data_ptr create_macro_data(std::unique_ptr<macro_param_data_single_dynamic> value) { return value; }

template<typename SYSTEM_VARIABLE_TYPE = system_variable, typename DATA>
std::pair<id_index, sys_sym_ptr> create_system_variable(id_index id, DATA mac_data, bool is_global)
{
    return { id, std::make_shared<SYSTEM_VARIABLE_TYPE>(id, create_macro_data(std::move(mac_data)), is_global) };
}
} // namespace

void hlasm_context::add_system_vars_to_scope(code_scope& scope)
{
    if (scope.is_in_macro())
    {
        {
            auto sect_name = ord_ctx.current_section() ? ord_ctx.current_section()->name : id_index();

            scope.system_variables.insert(create_system_variable(id_index("SYSECT"), sect_name.to_string(), false));
        }

        {
            std::string value = left_pad(std::to_string(SYSNDX_), 4, '0');

            scope.system_variables.insert(create_system_variable(id_index("SYSNDX"), std::move(value), false));
        }

        {
            std::string value = "";

            if (ord_ctx.current_section())
            {
                switch (ord_ctx.current_section()->kind)
                {
                    case context::section_kind::COMMON:
                        value = "COM";
                        break;
                    case context::section_kind::DUMMY:
                        value = "DSECT";
                        break;
                    case context::section_kind::READONLY:
                        value = "RSECT";
                        break;
                    case context::section_kind::EXECUTABLE:
                        value = "CSECT";
                        break;
                    default:
                        break;
                }
            }

            scope.system_variables.insert(create_system_variable(id_index("SYSSTYP"), std::move(value), false));
        }

        {
            std::string location_counter_name = "";

            if (ord_ctx.current_section())
            {
                location_counter_name = ord_ctx.current_section()->current_location_counter().name.to_string();
            }

            scope.system_variables.insert(
                create_system_variable(id_index("SYSLOC"), std::move(location_counter_name), false));
        }

        {
            std::string value = std::to_string(scope_stack_.size() - 1);

            scope.system_variables.insert(create_system_variable(id_index("SYSNEST"), std::move(value), false));
        }

        {
            std::vector<std::string> data;

            for (auto it = scope_stack_.rbegin(); it != scope_stack_.rend(); ++it)
            {
                if (it->is_in_macro())
                    data.push_back(it->this_macro->id.to_string());
                else
                    data.push_back("OPEN CODE");
            }

            scope.system_variables.insert(
                create_system_variable<system_variable_sysmac>(id_index("SYSMAC"), std::move(data), false));
        }

        {
            scope.system_variables.insert(create_system_variable(id_index("SYSIN_DSN"), asm_options_.sysin_dsn, false));
        }

        {
            scope.system_variables.insert(
                create_system_variable(id_index("SYSIN_MEMBER"), asm_options_.sysin_member, false));
        }

        {
            scope.system_variables.insert(create_system_variable(id_index("SYSCLOCK"),
                utils::timestamp::now().value_or(utils::timestamp(1900, 1, 1)).to_string(),
                false));
        }
    }
}

void hlasm_context::add_global_system_var_to_scope(id_index id, code_scope& scope) const
{
    auto glob = globals_.find(id);

    sys_sym_ptr temp = std::dynamic_pointer_cast<system_variable>(glob->second);

    scope.system_variables.try_emplace(glob->second->id, temp);
}

void hlasm_context::add_global_system_vars(code_scope& scope)
{
    if (!is_in_macro())
    {
        {
            auto now = utils::timestamp::now().value_or(utils::timestamp(1900, 1, 1));

            std::string datc_val;
            std::string date_val;
            datc_val.reserve(8);
            date_val.reserve(8);
            auto year = std::to_string(now.year());
            datc_val.append(year);

            if (now.month() < 10)
            {
                datc_val.push_back('0');
                date_val.push_back('0');
            }

            datc_val.append(std::to_string(now.month()));

            date_val.append(std::to_string(now.month()));
            date_val.push_back('/');

            if (now.day() < 10)
            {
                datc_val.push_back('0');
                date_val.push_back('0');
            }

            datc_val.append(std::to_string(now.day()));

            date_val.append(std::to_string(now.day()));
            date_val.push_back('/');
            date_val.append(year.c_str() + 2);

            {
                globals_.insert(
                    create_system_variable<system_variable>(id_index("SYSDATC"), std::move(datc_val), true));
            }

            {
                globals_.insert(
                    create_system_variable<system_variable>(id_index("SYSDATE"), std::move(date_val), true));
            }

            {
                std::string value;
                if (now.hour() < 10)
                    value.push_back('0');
                value.append(std::to_string(now.hour()));
                value.push_back(':');
                if (now.minute() < 10)
                    value.push_back('0');
                value.append(std::to_string(now.minute()));

                globals_.insert(create_system_variable<system_variable>(id_index("SYSTIME"), std::move(value), true));
            }
        }

        {
            globals_.insert(create_system_variable<system_variable>(
                id_index("SYSOPT_RENT"), std::to_string(asm_options_.sysopt_rent), true));
        }

        {
            globals_.insert(create_system_variable<system_variable>(
                id_index("SYSOPT_XOBJECT"), std::to_string(asm_options_.sysopt_xobject), true));
        }

        {
            globals_.insert(create_system_variable<system_variable>(id_index("SYSPARM"), asm_options_.sysparm, true));
        }

        {
            globals_.insert(create_system_variable<system_variable>(
                id_index("SYSSTMT"), std::make_unique<sysstmt_macro_param_data>(metrics, scope_stack_), true));
        }

        {
            globals_.insert(
                create_system_variable<system_variable>(id_index("SYSTEM_ID"), asm_options_.system_id, true));
        }

        {
            static constexpr const auto emulated_hlasm_sysver = "1.6.0";
            globals_.insert(create_system_variable<system_variable>(id_index("SYSVER"), emulated_hlasm_sysver, true));
        }

        {
            static constexpr const auto emulated_asm_name = "HIGH LEVEL ASSEMBLER";
            globals_.insert(create_system_variable<system_variable>(id_index("SYSASM"), emulated_asm_name, true));
        }

        {
            globals_.insert(create_system_variable<system_variable>(
                id_index("SYSM_SEV"), std::make_unique<integral_macro_param_data<unsigned, 3>>(mnote_last_max), true));
        }

        {
            globals_.insert(create_system_variable<system_variable>(
                id_index("SYSM_HSEV"), std::make_unique<integral_macro_param_data<unsigned, 3>>(mnote_max), true));
        }
    }

    add_global_system_var_to_scope(id_index("SYSDATC"), scope);
    add_global_system_var_to_scope(id_index("SYSDATE"), scope);
    add_global_system_var_to_scope(id_index("SYSTIME"), scope);
    add_global_system_var_to_scope(id_index("SYSOPT_RENT"), scope);
    add_global_system_var_to_scope(id_index("SYSOPT_XOBJECT"), scope);
    add_global_system_var_to_scope(id_index("SYSPARM"), scope);
    add_global_system_var_to_scope(id_index("SYSSTMT"), scope);
    add_global_system_var_to_scope(id_index("SYSTEM_ID"), scope);
    add_global_system_var_to_scope(id_index("SYSVER"), scope);
    add_global_system_var_to_scope(id_index("SYSASM"), scope);
    add_global_system_var_to_scope(id_index("SYSM_SEV"), scope);
    add_global_system_var_to_scope(id_index("SYSM_HSEV"), scope);
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
    init_instruction_map(opcode_mnemo_, *ids_, asm_options_.instr_set);

    add_global_system_vars(scope_stack_.emplace_back());
    visited_files_.insert(file_loc);
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

    if (snapshot.copy_frames.size() && is_in_macros)
        ++snapshot.copy_frames.back().statement_offset;

    return std::make_pair(std::move(statement_position), std::move(snapshot));
}

std::pair<source_position, source_snapshot> hlasm_context::get_end_snapshot() const
{
    context::source_position statement_position;
    statement_position.rewind_target = current_source().end_index;

    context::source_snapshot snapshot = current_source().create_snapshot();

    if (snapshot.copy_frames.size())
        ++snapshot.copy_frames.back().statement_offset;

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

id_storage& hlasm_context::ids() { return *ids_; }
const id_storage& hlasm_context::ids() const { return *ids_; }

std::shared_ptr<id_storage> hlasm_context::ids_ptr() { return ids_; }

const utils::resource::resource_location* hlasm_context::shared_resource_location(
    const utils::resource::resource_location& l)
{
    return std::to_address(m_resource_locations.emplace(l).first);
}
const utils::resource::resource_location* hlasm_context::shared_resource_location(
    utils::resource::resource_location&& l)
{
    return std::to_address(m_resource_locations.emplace(std::move(l)).first);
}

processing_stack_t hlasm_context::processing_stack()
{
    auto result = m_stack_tree.root();

    for (bool first = true; const auto& source : source_stack_)
    {
        result = m_stack_tree.step(processing_frame(source.current_instruction.pos,
                                       shared_resource_location(source.current_instruction.resource_loc),
                                       id_index()),
            result);
        for (const auto& member : source.copy_stack)
        {
            result = m_stack_tree.step(processing_frame(member.current_statement_position(),
                                           shared_resource_location(member.definition_location()->resource_loc),
                                           member.name()),
                result);
        }

        if (first) // append macros immediately after ordinary processing
        {
            first = false;
            for (size_t j = 1; j < scope_stack_.size(); ++j)
            {
                auto offs = scope_stack_[j].this_macro->current_statement;

                for (const auto& nest : scope_stack_[j].this_macro->copy_nests[offs])
                    result = m_stack_tree.step(
                        processing_frame(
                            nest.loc.pos, shared_resource_location(nest.loc.resource_loc), nest.member_name),
                        result);
            }
        }
    }

    return result;
}

processing_frame hlasm_context::processing_stack_top(bool consider_macros)
{
    // this is an inverted version of the loop from processing_stack()
    // terminating after finding the first (originally last) element

    assert(!source_stack_.empty());

    if (consider_macros && source_stack_.size() == 1 && scope_stack_.size() > 1)
    {
        const auto& last_macro = scope_stack_.back().this_macro;

        if (const auto& nest = last_macro->copy_nests[last_macro->current_statement]; !nest.empty())
        {
            const auto& last_copy = nest.back();
            return processing_frame(
                last_copy.loc.pos, shared_resource_location(last_copy.loc.resource_loc), last_copy.member_name);
        }
    }

    const auto& last_source = source_stack_.back();
    if (!last_source.copy_stack.empty())
    {
        const auto& last_member = last_source.copy_stack.back();
        return processing_frame(last_member.current_statement_position(),
            shared_resource_location(last_member.definition_location()->resource_loc),
            last_member.name());
    }

    return processing_frame(last_source.current_instruction.pos,
        shared_resource_location(last_source.current_instruction.resource_loc),
        id_index());
}

processing_stack_details_t hlasm_context::processing_stack_details()
{
    std::vector<processing_frame_details> res;

    for (bool first = true; const auto& source : source_stack_)
    {
        res.emplace_back(source.current_instruction.pos,
            shared_resource_location(source.current_instruction.resource_loc),
            scope_stack_.front(),
            file_processing_type::OPENCODE,
            id_index());
        for (const auto& member : source.copy_stack)
        {
            res.emplace_back(member.current_statement_position(),
                shared_resource_location(member.definition_location()->resource_loc),
                scope_stack_.front(),
                file_processing_type::COPY,
                member.name());
        }

        if (first) // append macros immediately after ordinary processing
        {
            first = false;
            for (size_t j = 1; j < scope_stack_.size(); ++j)
            {
                auto offs = scope_stack_[j].this_macro->current_statement;

                for (auto type = file_processing_type::MACRO;
                     const auto& nest : scope_stack_[j].this_macro->copy_nests[offs])
                {
                    res.emplace_back(nest.loc.pos,
                        shared_resource_location(nest.loc.resource_loc),
                        scope_stack_[j],
                        type,
                        nest.member_name);
                    type = file_processing_type::COPY;
                }
            }
        }
    }

    return res;
}

location hlasm_context::current_statement_location() const
{
    if (source_stack_.size() > 1 || scope_stack_.size() == 1)
    {
        if (source_stack_.back().copy_stack.size())
        {
            const auto& member = source_stack_.back().copy_stack.back();

            return location(member.current_statement_position(), member.definition_location()->resource_loc);
        }
        else
            return source_stack_.back().current_instruction;
    }
    else
    {
        const auto& mac_invo = scope_stack_.back().this_macro;

        return mac_invo->copy_nests[mac_invo->current_statement].back().loc;
    }
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

void hlasm_context::fill_metrics_files() { metrics.files = visited_files_.size(); }

const hlasm_context::global_variable_storage& hlasm_context::globals() const { return globals_; }

var_sym_ptr hlasm_context::get_var_sym(id_index name) const
{
    const auto* scope = curr_scope();
    if (auto tmp = scope->variables.find(name); tmp != scope->variables.end())
        return tmp->second;

    if (auto s_tmp = scope->system_variables.find(name); s_tmp != scope->system_variables.end())
        return s_tmp->second;

    if (scope->is_in_macro())
    {
        auto m_tmp = scope->this_macro->named_params.find(name);
        if (m_tmp != scope->this_macro->named_params.end())
            return m_tmp->second;
    }

    return var_sym_ptr();
}

void hlasm_context::add_opencode_sequence_symbol(std::unique_ptr<opencode_sequence_symbol> seq_sym)
{
    auto& opencode = scope_stack_.front();
    opencode.sequence_symbols.try_emplace(seq_sym->name, std::move(seq_sym));
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
        found = scope->sequence_symbols.find(name);
        end = scope->sequence_symbols.end();
    }

    if (found != end)
        return found->second.get();
    else
        return nullptr;
}

const sequence_symbol* hlasm_context::get_opencode_sequence_symbol(id_index name) const
{
    if (const auto& sym = scope_stack_.front().sequence_symbols.find(name);
        sym != scope_stack_.front().sequence_symbols.end())
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

const opcode_t* hlasm_context::find_opcode_mnemo(id_index name, opcode_generation gen) const
{
    auto it = opcode_mnemo_.upper_bound({ name, gen });
    if (it == opcode_mnemo_.begin())
        return nullptr;
    it = std::prev(it);
    if (it->first.first != name)
        return nullptr;
    if (it->first.second > gen)
        return nullptr;
    return &it->second;
}

void hlasm_context::add_mnemonic(id_index mnemo, id_index op_code)
{
    auto tmp = find_opcode_mnemo(op_code, opcode_generation::current);
    assert(tmp && *tmp); // mnemonic was not removed

    opcode_mnemo_.try_emplace({ mnemo, ++m_current_opcode_generation }, *tmp);
}

void hlasm_context::remove_mnemonic(id_index mnemo)
{
    [[maybe_unused]] const opcode_t* it;
    assert((it = find_opcode_mnemo(mnemo, opcode_generation::current)) && *it);
    opcode_mnemo_.try_emplace({ mnemo, ++m_current_opcode_generation }, opcode_t());
}

opcode_t hlasm_context::get_operation_code(id_index symbol, opcode_generation gen) const
{
    if (auto it = find_opcode_mnemo(symbol, gen); it)
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
                return std::string { (char)ebcdic_encoding::e2a[attr_val] };
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
    std::string operator()(const assembler_instruction*) const { return "A"; }
    std::string operator()(const ca_instruction*) const { return "A"; }
    std::string operator()(const mnemonic_code*) const { return "E"; }
    std::string operator()(const machine_instruction*) const { return "O"; }
    std::string operator()(const macro_def_ptr&) const { return "M"; }
    template<typename T>
    std::string operator()(const T&) const
    {
        return "U";
    }
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
    std::unordered_set<copy_member_ptr> used_copy_members)
{
    auto result = std::make_shared<macro_definition>(name,
        label_param_name,
        std::move(params),
        std::move(definition),
        std::move(copy_nests),
        std::move(labels),
        std::move(definition_location),
        std::move(used_copy_members));
    add_macro(result);
    return result;
}

void hlasm_context::add_macro(macro_def_ptr macro)
{
    auto next_gen = ++m_current_opcode_generation;
    const auto& m = macros_.try_emplace({ macro->id, next_gen }, std::move(macro)).first->second;
    opcode_mnemo_.try_emplace({ m->id, next_gen }, opcode_t { m->id, m });
};

const hlasm_context::macro_storage& hlasm_context::macros() const { return macros_; }

const macro_def_ptr* hlasm_context::find_macro(id_index name, opcode_generation gen) const
{
    if (auto it = macros_.upper_bound({ name, gen });
        it == macros_.begin() || (it = std::prev(it))->first.first != name || it->first.second > gen)
        return nullptr;
    else
        return &it->second;
}

const hlasm_context::opcode_map& hlasm_context::opcode_mnemo_storage() const { return opcode_mnemo_; }

macro_def_ptr hlasm_context::get_macro_definition(id_index name, context::opcode_generation gen) const
{
    if (auto mnem = find_opcode_mnemo(name, gen);
        mnem && std::holds_alternative<context::macro_def_ptr>(mnem->opcode_detail))
        return std::get<context::macro_def_ptr>(mnem->opcode_detail);

    return macro_def_ptr();
}

bool hlasm_context::is_in_macro() const { return scope_stack_.back().is_in_macro(); }

macro_invo_ptr hlasm_context::enter_macro(id_index name, macro_data_ptr label_param_data, std::vector<macro_arg> params)
{
    assert(SYSNDX_ <= SYSNDX_limit);

    macro_def_ptr macro_def = get_macro_definition(name);
    assert(macro_def);

    if (label_param_data)
    {
        if (auto label = label_param_data->get_value(); lexing::is_valid_symbol_name(label))
            ord_ctx.symbol_mentioned_on_macro(ids().add(std::move(label)));
    }

    auto invo = macro_def->call(std::move(label_param_data), std::move(params), id_storage::well_known::SYSLIST);
    auto& new_scope = scope_stack_.emplace_back(invo, macro_def);
    add_system_vars_to_scope(new_scope);
    add_global_system_vars(new_scope);

    visited_files_.insert(macro_def->definition_location.resource_loc);

    ++SYSNDX_;
    mnote_last_max = 0;

    return invo;
}

void hlasm_context::leave_macro()
{
    mnote_last_max = scope_stack_.back().mnote_max_in_scope;
    scope_stack_.pop_back();
}

macro_invo_ptr hlasm_context::this_macro() const
{
    if (is_in_macro())
        return curr_scope()->this_macro;
    return macro_invo_ptr();
}

const utils::resource::resource_location& hlasm_context::opencode_location() const { return opencode_file_location_; }

const std::set<utils::resource::resource_location>& hlasm_context::get_visited_files() const { return visited_files_; }

copy_member_ptr hlasm_context::add_copy_member(
    id_index member, statement_block definition, location definition_location)
{
    auto& copydef = copy_members_[member];
    if (!copydef)
        copydef = std::make_shared<copy_member>(member, std::move(definition), definition_location);
    visited_files_.insert(std::move(definition_location.resource_loc));

    return copydef;
}

void hlasm_context::add_copy_member(copy_member_ptr member)
{
    visited_files_.insert(member->definition_location.resource_loc);
    copy_members_[member->name] = std::move(member);
}


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

void hlasm_context::add_preprocessor_dependency(const utils::resource::resource_location& file_loc)
{
    visited_files_.emplace(file_loc);
}

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

void hlasm_context::using_resolve(diagnostic_s_consumer& diag, const library_info& li)
{
    m_usings->resolve_all(ord_ctx, diag, li);
}

index_t<using_collection> hlasm_context::using_current() const { return m_active_usings.back(); }

hlasm_context::name_result hlasm_context::try_get_symbol_name(std::string_view symbol)
{
    if (symbol.empty() || symbol.size() > 63 || isdigit((unsigned char)symbol.front()))
        return std::make_pair(false, context::id_index());

    for (const auto& c : symbol)
        if (!lexing::lexer::ord_char(c))
            return std::make_pair(false, context::id_index());

    return std::make_pair(true, ids().add(symbol));
}

SET_t get_var_sym_value(const hlasm_context& hlasm_ctx,
    id_index name,
    std::span<const context::A_t> subscript,
    range symbol_range,
    diagnostic_op_consumer& diags)
{
    auto var = hlasm_ctx.get_var_sym(name);
    if (!test_symbol_for_read(var, subscript, symbol_range, diags, name.to_string_view()))
        return SET_t();

    if (auto set_sym = var->access_set_symbol_base())
    {
        size_t idx = 0;

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
            idx = (size_t)(subscript.front() - 1);

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
        std::vector<size_t> tmp;
        for (auto& v : subscript)
        {
            tmp.push_back((size_t)v);
        }
        return mac_par->get_value(tmp);
    }
    return SET_t();
}

bool test_symbol_for_read(const var_sym_ptr& var,
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
} // namespace hlasm_plugin::parser_library::context
