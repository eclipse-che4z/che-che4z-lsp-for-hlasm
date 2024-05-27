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

#include "instruction.h"

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <utility>

#include "checking/diagnostic_collector.h"

using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library::checking;
using namespace hlasm_plugin::parser_library;

namespace {
constexpr z_arch_affiliation operator|(z_arch_affiliation a, z_arch_affiliation b)
{
    if (a != z_arch_affiliation::NO_AFFILIATION && b != z_arch_affiliation::NO_AFFILIATION)
    {
        return std::min(a, b);
    }
    else
    {
        return std::max(a, b);
    }
}

constexpr instruction_set_affiliation operator|(instruction_set_affiliation a, z_arch_affiliation z_affil)
{
    a.z_arch = a.z_arch | z_affil;

    return a;
}

constexpr instruction_set_affiliation operator|(instruction_set_affiliation a, instruction_set_affiliation b)
{
    instruction_set_affiliation result {};

    result.z_arch = a.z_arch | b.z_arch;
    result.esa = a.esa | b.esa;
    result.xa = a.xa | b.xa;
    result._370 = a._370 | b._370;
    result.dos = a.dos | b.dos;
    result.uni = a.uni | b.uni;

    return result;
}

// clang-format off
constexpr auto ESA       = instruction_set_affiliation{z_arch_affiliation::NO_AFFILIATION, 1, 0, 0, 0, 0};
constexpr auto XA        = instruction_set_affiliation{z_arch_affiliation::NO_AFFILIATION, 0, 1, 0, 0, 0};
constexpr auto _370      = instruction_set_affiliation{z_arch_affiliation::NO_AFFILIATION, 0, 0, 1, 0, 0};
constexpr auto DOS       = instruction_set_affiliation{z_arch_affiliation::NO_AFFILIATION, 0, 0, 0, 1, 0};
constexpr auto UNI       = instruction_set_affiliation{z_arch_affiliation::NO_AFFILIATION, 0, 0, 0, 0, 1};

constexpr auto ESA_XA                       = ESA | XA;
constexpr auto ESA_XA_370                   = ESA | XA | _370;
constexpr auto UNI_370                      = UNI | _370;
constexpr auto UNI_370_DOS                  = UNI | _370 | DOS;
constexpr auto UNI_ESA_SINCE_ZOP            = UNI | ESA | z_arch_affiliation::SINCE_ZOP;
constexpr auto UNI_ESA_XA_370_DOS_SINCE_ZOP = UNI | ESA | XA | _370 | DOS | z_arch_affiliation::SINCE_ZOP;
constexpr auto UNI_ESA_XA_370_SINCE_Z13     = UNI | ESA | XA | _370 | z_arch_affiliation::SINCE_Z13;
constexpr auto UNI_ESA_XA_370_SINCE_Z15     = UNI | ESA | XA | _370 | z_arch_affiliation::SINCE_Z15;
constexpr auto UNI_ESA_XA_370_SINCE_ZOP     = UNI | ESA | XA | _370 | z_arch_affiliation::SINCE_ZOP;
constexpr auto UNI_ESA_XA_SINCE_ZOP         = UNI | ESA | XA | z_arch_affiliation::SINCE_ZOP;
constexpr auto UNI_SINCE_YOP                = UNI | z_arch_affiliation::SINCE_YOP;
constexpr auto UNI_SINCE_Z10                = UNI | z_arch_affiliation::SINCE_Z10;
constexpr auto UNI_SINCE_Z11                = UNI | z_arch_affiliation::SINCE_Z11;
constexpr auto UNI_SINCE_Z12                = UNI | z_arch_affiliation::SINCE_Z12;
constexpr auto UNI_SINCE_Z13                = UNI | z_arch_affiliation::SINCE_Z13;
constexpr auto UNI_SINCE_Z14                = UNI | z_arch_affiliation::SINCE_Z14;
constexpr auto UNI_SINCE_Z15                = UNI | z_arch_affiliation::SINCE_Z15;
constexpr auto UNI_SINCE_Z16                = UNI | z_arch_affiliation::SINCE_Z16;
constexpr auto UNI_SINCE_Z9                 = UNI | z_arch_affiliation::SINCE_Z9;
constexpr auto UNI_SINCE_ZOP                = UNI | z_arch_affiliation::SINCE_ZOP;
// clang-format on
} // namespace

std::string_view instruction::mach_format_to_string(mach_format f) noexcept
{
    switch (f)
    {
        case mach_format::E:
            return "E";
        case mach_format::I:
            return "I";
        case mach_format::IE:
            return "IE";
        case mach_format::MII:
            return "MII";
        case mach_format::RI_a:
            return "RI-a";
        case mach_format::RI_b:
            return "RI-b";
        case mach_format::RI_c:
            return "RI-c";
        case mach_format::RIE_a:
            return "RIE-a";
        case mach_format::RIE_b:
            return "RIE-b";
        case mach_format::RIE_c:
            return "RIE-c";
        case mach_format::RIE_d:
            return "RIE-d";
        case mach_format::RIE_e:
            return "RIE-e";
        case mach_format::RIE_f:
            return "RIE-f";
        case mach_format::RIE_g:
            return "RIE-g";
        case mach_format::RIL_a:
            return "RIL-a";
        case mach_format::RIL_b:
            return "RIL-b";
        case mach_format::RIL_c:
            return "RIL-c";
        case mach_format::RIS:
            return "RIS";
        case mach_format::RR:
            return "RR";
        case mach_format::RRD:
            return "RRD";
        case mach_format::RRE:
            return "RRE";
        case mach_format::RRF_a:
            return "RRF-a";
        case mach_format::RRF_b:
            return "RRF-b";
        case mach_format::RRF_c:
            return "RRF-c";
        case mach_format::RRF_d:
            return "RRF-d";
        case mach_format::RRF_e:
            return "RRF-e";
        case mach_format::RRS:
            return "RRS";
        case mach_format::RS_a:
            return "RS-a";
        case mach_format::RS_b:
            return "RS-b";
        case mach_format::RSI:
            return "RSI";
        case mach_format::RSL_a:
            return "RSL-a";
        case mach_format::RSL_b:
            return "RSL-b";
        case mach_format::RSY_a:
            return "RSY-a";
        case mach_format::RSY_b:
            return "RSY-b";
        case mach_format::RX_a:
            return "RX-a";
        case mach_format::RX_b:
            return "RX-b";
        case mach_format::RXE:
            return "RXE";
        case mach_format::RXF:
            return "RXF";
        case mach_format::RXY_a:
            return "RXY-a";
        case mach_format::RXY_b:
            return "RXY-b";
        case mach_format::S:
            return "S";
        case mach_format::SI:
            return "SI";
        case mach_format::DIAGNOSE:
            return "DIAGNOSE";
        case mach_format::SIL:
            return "SIL";
        case mach_format::SIY:
            return "SIY";
        case mach_format::SMI:
            return "SMI";
        case mach_format::SS_a:
            return "SS-a";
        case mach_format::SS_b:
            return "SS-b";
        case mach_format::SS_c:
            return "SS-c";
        case mach_format::SS_d:
            return "SS-d";
        case mach_format::SS_e:
            return "SS-e";
        case mach_format::SS_f:
            return "SS-f";
        case mach_format::SSE:
            return "SSE";
        case mach_format::SSF:
            return "SSF";
        case mach_format::VRI_a:
            return "VRI-a";
        case mach_format::VRI_b:
            return "VRI-b";
        case mach_format::VRI_c:
            return "VRI-c";
        case mach_format::VRI_d:
            return "VRI-d";
        case mach_format::VRI_e:
            return "VRI-e";
        case mach_format::VRI_f:
            return "VRI-f";
        case mach_format::VRI_g:
            return "VRI-g";
        case mach_format::VRI_h:
            return "VRI-h";
        case mach_format::VRI_i:
            return "VRI-i";
        case mach_format::VRR_a:
            return "VRR-a";
        case mach_format::VRR_b:
            return "VRR-b";
        case mach_format::VRR_c:
            return "VRR-c";
        case mach_format::VRR_d:
            return "VRR-d";
        case mach_format::VRR_e:
            return "VRR-e";
        case mach_format::VRR_f:
            return "VRR-f";
        case mach_format::VRR_g:
            return "VRR-g";
        case mach_format::VRR_h:
            return "VRR-h";
        case mach_format::VRR_i:
            return "VRR-i";
        case mach_format::VRR_j:
            return "VRR-j";
        case mach_format::VRR_k:
            return "VRR-k";
        case mach_format::VRS_a:
            return "VRS-a";
        case mach_format::VRS_b:
            return "VRS-b";
        case mach_format::VRS_c:
            return "VRS-c";
        case mach_format::VRS_d:
            return "VRS-d";
        case mach_format::VSI:
            return "VSI";
        case mach_format::VRV:
            return "VRV";
        case mach_format::VRX:
            return "VRX";
        default:
            assert(false);
            return "";
    }
}

constexpr ca_instruction ca_instructions[] = {
    { "ACTR", false },
    { "AEJECT", true },
    { "AGO", false },
    { "AIF", false },
    { "ANOP", true },
    { "AREAD", false },
    { "ASPACE", false },
    { "GBLA", false },
    { "GBLB", false },
    { "GBLC", false },
    { "LCLA", false },
    { "LCLB", false },
    { "LCLC", false },
    { "MACRO", true },
    { "MEND", true },
    { "MEXIT", true },
    { "MHELP", false },
    { "SETA", false },
    { "SETB", false },
    { "SETC", false },
};

static_assert(std::ranges::is_sorted(ca_instructions, {}, &ca_instruction::name));

const ca_instruction* instruction::find_ca_instructions(std::string_view name) noexcept
{
    auto it = std::ranges::lower_bound(ca_instructions, name, {}, &ca_instruction::name);

    if (it == std::ranges::end(ca_instructions) || it->name() != name)
        return nullptr;
    return std::to_address(it);
}

const ca_instruction& instruction::get_ca_instructions(std::string_view name) noexcept
{
    auto result = find_ca_instructions(name);
    assert(result);
    return *result;
}

std::span<const ca_instruction> instruction::all_ca_instructions() noexcept { return ca_instructions; }

constexpr assembler_instruction assembler_instructions[] = {
    { "*PROCESS", 1, -1, false, "" }, // TO DO
    { "ACONTROL", 1, -1, false, "<selection>+" },
    { "ADATA", 5, 5, false, "value1,value2,value3,value4,character_string" },
    { "AINSERT", 2, 2, false, "'record',BACK|FRONT" },
    { "ALIAS", 1, 1, false, "alias_string" },
    { "AMODE", 1, 1, false, "amode_option" },
    { "CATTR", 1, -1, false, "attribute+" },
    { "CCW", 4, 4, true, "command_code,data_address,flags,data_count" },
    { "CCW0", 4, 4, true, "command_code,data_address,flags,data_count" },
    { "CCW1", 4, 4, true, "command_code,data_address,flags,data_count" },
    { "CEJECT", 0, 1, true, "?number_of_lines" },
    { "CNOP", 2, 2, true, "byte,boundary" },
    { "COM", 0, 0, false, "" },
    { "COPY", 1, 1, false, "member" },
    { "CSECT", 0, 0, false, "" },
    { "CXD", 0, 0, false, "" },
    { "DC", 1, -1, true, "<operand>+" },
    { "DROP", 0, -1, true, "?<<base_register|label>+>", true },
    { "DS", 1, -1, true, "<operand>+" },
    { "DSECT", 0, 0, false, "" },
    { "DXD", 1, -1, true, "<operand>+" },
    { "EJECT", 0, 0, false, "" },
    { "END", 0, 2, true, "?expression,?language" },
    { "ENTRY", 1, -1, true, "entry_point+" },
    { "EQU",
        1,
        5,
        true,
        "value,?<length_attribute_value>,?<type_attribute_value>,?<program_type_value>,?<assembler_type_value>" },
    { "EXITCTL", 2, 5, false, "exit_type,control_value+" },
    { "EXTRN", 1, -1, false, "<external_symbol>+|PART(<external_symbol>+" },
    { "ICTL", 1, 3, false, "begin,?<end>,?<continue>" },
    { "ISEQ", 0, 2, false, "?<left,right>" },
    { "LOCTR", 0, 0, false, "" },
    { "LTORG", 0, 0, false, "" },
    { "MNOTE", 1, 2, false, "?<<severity|*|>,>message" },
    { "OPSYN", 0, 1, false, "?operation_code_2" },
    { "ORG", 0, 3, true, "expression?<,boundary?<,offset>>" },
    { "POP", 1, 4, false, "<PRINT|USING|ACONTROL>+,?NOPRINT" },
    { "PRINT", 1, -1, false, "operand+" },
    { "PUNCH", 1, 1, false, "string" },
    { "PUSH", 1, 4, false, "<PRINT|USING|ACONTROL>+,?NOPRINT" },
    { "REPRO", 0, 0, false, "" },
    { "RMODE", 1, 1, false, "rmode_option" },
    { "RSECT", 0, 0, false, "" },
    { "SPACE", 0, 1, true, "?number_of_lines" },
    { "START", 0, 1, true, "?expression" },
    { "TITLE", 1, 1, false, "title_string" },
    { "USING", 2, 17, true, "operand+", true },
    { "WXTRN", 1, -1, false, "<external_symbol>+|PART(<external_symbol>+" },
    { "XATTR", 1, -1, false, "attribute+" },

};

static_assert(std::ranges::is_sorted(assembler_instructions, {}, &assembler_instruction::name));

const assembler_instruction* instruction::find_assembler_instructions(std::string_view instr) noexcept
{
    auto it = std::ranges::lower_bound(assembler_instructions, instr, {}, &assembler_instruction::name);
    if (it == std::ranges::end(assembler_instructions) || it->name() != instr)
        return nullptr;
    return std::to_address(it);
}

const assembler_instruction& instruction::get_assembler_instructions(std::string_view instr) noexcept
{
    auto result = find_assembler_instructions(instr);
    assert(result);
    return *result;
}

std::span<const assembler_instruction> instruction::all_assembler_instructions() noexcept
{
    return assembler_instructions;
}

bool hlasm_plugin::parser_library::context::machine_instruction::check(std::string_view name_of_instruction,
    std::span<const checking::machine_operand* const> to_check,
    const range& stmt_range,
    const diagnostic_collector& add_diagnostic) const
{
    // check size of operands
    int diff = (int)m_operand_len - (int)to_check.size();
    if (diff > m_optional_op_count || diff < 0)
    {
        add_diagnostic(diagnostic_op::error_optional_number_of_operands(
            name_of_instruction, m_optional_op_count, (int)m_operand_len, stmt_range));
        return false;
    }
    bool error = false;
    for (const auto* fmt = m_operands; const auto* op : to_check)
    {
        assert(op != nullptr);
        if (auto diag = op->check(*fmt++, name_of_instruction, stmt_range); diag.has_value())
        {
            add_diagnostic(std::move(diag).value());
            error = true;
        }
    };
    return !error;
}

template<mach_format F, const machine_operand_format&... Ops>
class instruction_format_definition_factory
{
    static constexpr std::array<machine_operand_format, sizeof...(Ops)> format = { Ops... };

public:
    static constexpr instruction_format_definition def() { return { { format.data(), sizeof...(Ops) }, F }; }
};
template<mach_format F>
class instruction_format_definition_factory<F>
{
public:
    static constexpr instruction_format_definition def() { return { {}, F }; }
};

namespace {
struct
{
} constexpr privileged;
struct
{
} constexpr privileged_conditionally;
struct
{
} constexpr has_parameter_list;

template<int arg, int nonzero>
struct branch_argument_t
{};

template<typename T>
struct is_branch_argument_t : std::false_type
{
    using branch_t = branch_argument_t<0, 0>;
};
template<int arg, int nonzero>
struct is_branch_argument_t<branch_argument_t<arg, nonzero>> : std::true_type
{
    using branch_t = branch_argument_t<arg, nonzero>;
};

template<int... arg, int... nonzero>
constexpr branch_info_argument select_branch_argument(branch_argument_t<arg, nonzero>...)
{
    constexpr auto res = []() {
        const int args[] = { arg..., 0 };
        const int nonzeros[] = { nonzero..., 0 };
        for (size_t i = 0; i < std::size(args); ++i)
            if (args[i])
                return std::make_pair(args[i], nonzeros[i]);
        return std::make_pair(0, 0);
    }();
    return branch_info_argument { (signed char)res.first, (unsigned char)res.second };
}

constexpr branch_argument_t<-1, 0> branch_argument_unknown;
template<unsigned char nonzero>
requires(nonzero < 4) constexpr branch_argument_t<-1, 1 + nonzero> branch_argument_unknown_nonzero;
template<unsigned char arg>
requires(arg < 4) constexpr branch_argument_t<1 + arg, 0> branch_argument;
template<unsigned char arg, unsigned char nonzero>
requires(arg < 4 && nonzero < 4) constexpr branch_argument_t<1 + arg, 1 + nonzero> branch_argument_nonzero;

struct cc_index
{
    unsigned char value;
};

template<typename... Args>
struct make_machine_instruction_details_args_validator
{
    static constexpr size_t p = (0 + ... + std::is_same_v<Args, std::decay_t<decltype(privileged)>>);
    static constexpr size_t p_c = (0 + ... + std::is_same_v<Args, std::decay_t<decltype(privileged_conditionally)>>);
    static constexpr size_t pl = (0 + ... + std::is_same_v<Args, std::decay_t<decltype(has_parameter_list)>>);
    static constexpr size_t cc = (0 + ... + std::is_same_v<Args, cc_index>);
    static constexpr branch_info_argument ba =
        select_branch_argument(typename is_branch_argument_t<Args>::branch_t()...);
    static constexpr bool value =
        !(p && p_c) && p <= 1 && p_c <= 1 && pl <= 1 && cc <= 1 && (0 + ... + is_branch_argument_t<Args>::value) <= 1;
};

struct
{
    constexpr unsigned char operator()(const cc_index& val) const noexcept { return val.value; }
    constexpr unsigned char operator()(const auto&) const noexcept { return 0; }
} constexpr cc_visitor;

template<size_t n, typename... Args>
constexpr machine_instruction_details make_machine_instruction_details(const char (&name)[n], Args&&... args) noexcept
    requires(n > 1 && n < 256 && make_machine_instruction_details_args_validator<std::decay_t<Args>...>::value)
{
    using A = make_machine_instruction_details_args_validator<std::decay_t<Args>...>;
    return machine_instruction_details {
        name, n - 1, static_cast<unsigned char>((0 + ... + cc_visitor(args))), A::p > 0, A::p_c > 0, A::pl > 0, A::ba
    };
}

enum class condition_code_explanation_id : unsigned char
{
#define DEFINE_CC_SET(name, ...) name,
#include "instruction_details.h"
};


#define DEFINE_CC_SET(name, ...)                                                                                       \
    constexpr const auto name = cc_index { static_cast<unsigned char>(condition_code_explanation_id::name) };
#include "instruction_details.h"
} // namespace

constinit const condition_code_explanation hlasm_plugin::parser_library::context::condition_code_explanations[] = {
#define DEFINE_CC_SET(name, ...) condition_code_explanation(__VA_ARGS__),
#include "instruction_details.h"
};

#define DEFINE_INSTRUCTION_FORMAT(name, format, ...)                                                                   \
    constexpr auto name = instruction_format_definition_factory<format __VA_OPT__(, ) __VA_ARGS__>::def();
#include "instruction_details.h"

#define DEFINE_INSTRUCTION(name, format, page, iset, description, ...)                                                 \
    { #name, format, page, iset, make_machine_instruction_details(description __VA_OPT__(, ) __VA_ARGS__) },
constexpr machine_instruction machine_instructions[] = {
#include "instruction_details.h"
};

static_assert(std::ranges::is_sorted(machine_instructions, {}, &machine_instruction::name));

const machine_instruction* instruction::find_machine_instructions(std::string_view name) noexcept
{
    auto it = std::ranges::lower_bound(machine_instructions, name, {}, &machine_instruction::name);
    if (it == std::ranges::end(machine_instructions) || it->name() != name)
        return nullptr;
    return std::to_address(it);
}

constexpr const machine_instruction* find_mi(std::string_view name)
{
    auto it = std::ranges::lower_bound(machine_instructions, name, {}, &machine_instruction::name);
    assert(it != std::ranges::end(machine_instructions) && it->name() == name);
    return std::to_address(it);
}

const machine_instruction& instruction::get_machine_instructions(std::string_view name) noexcept
{
    auto mi = find_machine_instructions(name);
    assert(mi);
    return *mi;
}

std::span<const machine_instruction> instruction::all_machine_instructions() noexcept { return machine_instructions; }

constexpr auto mi_BC = find_mi("BC");
constexpr auto mi_BCR = find_mi("BCR");
constexpr auto mi_BIC = find_mi("BIC");
constexpr auto mi_BRAS = find_mi("BRAS");
constexpr auto mi_BRASL = find_mi("BRASL");
constexpr auto mi_BRC = find_mi("BRC");
constexpr auto mi_BRCL = find_mi("BRCL");
constexpr auto mi_BRCT = find_mi("BRCT");
constexpr auto mi_BRCTG = find_mi("BRCTG");
constexpr auto mi_BRCTH = find_mi("BRCTH");
constexpr auto mi_BRXH = find_mi("BRXH");
constexpr auto mi_BRXHG = find_mi("BRXHG");
constexpr auto mi_BRXLE = find_mi("BRXLE");
constexpr auto mi_BRXLG = find_mi("BRXLG");
constexpr auto mi_CGIB = find_mi("CGIB");
constexpr auto mi_CGIJ = find_mi("CGIJ");
constexpr auto mi_CGIT = find_mi("CGIT");
constexpr auto mi_CGRB = find_mi("CGRB");
constexpr auto mi_CGRJ = find_mi("CGRJ");
constexpr auto mi_CGRT = find_mi("CGRT");
constexpr auto mi_CIB = find_mi("CIB");
constexpr auto mi_CIJ = find_mi("CIJ");
constexpr auto mi_CIT = find_mi("CIT");
constexpr auto mi_CLFIT = find_mi("CLFIT");
constexpr auto mi_CLGIB = find_mi("CLGIB");
constexpr auto mi_CLGIJ = find_mi("CLGIJ");
constexpr auto mi_CLGIT = find_mi("CLGIT");
constexpr auto mi_CLGRB = find_mi("CLGRB");
constexpr auto mi_CLGRJ = find_mi("CLGRJ");
constexpr auto mi_CLGRT = find_mi("CLGRT");
constexpr auto mi_CLGT = find_mi("CLGT");
constexpr auto mi_CLIB = find_mi("CLIB");
constexpr auto mi_CLIJ = find_mi("CLIJ");
constexpr auto mi_CLRB = find_mi("CLRB");
constexpr auto mi_CLRJ = find_mi("CLRJ");
constexpr auto mi_CLRT = find_mi("CLRT");
constexpr auto mi_CLT = find_mi("CLT");
constexpr auto mi_CRB = find_mi("CRB");
constexpr auto mi_CRJ = find_mi("CRJ");
constexpr auto mi_CRT = find_mi("CRT");
constexpr auto mi_IILF = find_mi("IILF");
constexpr auto mi_LLILF = find_mi("LLILF");
constexpr auto mi_LOC = find_mi("LOC");
constexpr auto mi_LOCFH = find_mi("LOCFH");
constexpr auto mi_LOCFHR = find_mi("LOCFHR");
constexpr auto mi_LOCG = find_mi("LOCG");
constexpr auto mi_LOCGHI = find_mi("LOCGHI");
constexpr auto mi_LOCGR = find_mi("LOCGR");
constexpr auto mi_LOCHHI = find_mi("LOCHHI");
constexpr auto mi_LOCHI = find_mi("LOCHI");
constexpr auto mi_LOCR = find_mi("LOCR");
constexpr auto mi_NOGRK = find_mi("NOGRK");
constexpr auto mi_NORK = find_mi("NORK");
constexpr auto mi_RISBG = find_mi("RISBG");
constexpr auto mi_RISBGN = find_mi("RISBGN");
constexpr auto mi_RISBHG = find_mi("RISBHG");
constexpr auto mi_RISBLG = find_mi("RISBLG");
constexpr auto mi_RNSBG = find_mi("RNSBG");
constexpr auto mi_ROSBG = find_mi("ROSBG");
constexpr auto mi_RXSBG = find_mi("RXSBG");
constexpr auto mi_SELFHR = find_mi("SELFHR");
constexpr auto mi_SELGR = find_mi("SELGR");
constexpr auto mi_SELR = find_mi("SELR");
constexpr auto mi_STOC = find_mi("STOC");
constexpr auto mi_STOCFH = find_mi("STOCFH");
constexpr auto mi_STOCG = find_mi("STOCG");
constexpr auto mi_VA = find_mi("VA");
constexpr auto mi_VAC = find_mi("VAC");
constexpr auto mi_VACC = find_mi("VACC");
constexpr auto mi_VACCC = find_mi("VACCC");
constexpr auto mi_VAVG = find_mi("VAVG");
constexpr auto mi_VAVGL = find_mi("VAVGL");
constexpr auto mi_VCEQ = find_mi("VCEQ");
constexpr auto mi_VCFPL = find_mi("VCFPL");
constexpr auto mi_VCFPS = find_mi("VCFPS");
constexpr auto mi_VCH = find_mi("VCH");
constexpr auto mi_VCHL = find_mi("VCHL");
constexpr auto mi_VCLFP = find_mi("VCLFP");
constexpr auto mi_VCLZ = find_mi("VCLZ");
constexpr auto mi_VCSFP = find_mi("VCSFP");
constexpr auto mi_VCTZ = find_mi("VCTZ");
constexpr auto mi_VEC = find_mi("VEC");
constexpr auto mi_VECL = find_mi("VECL");
constexpr auto mi_VERIM = find_mi("VERIM");
constexpr auto mi_VERLL = find_mi("VERLL");
constexpr auto mi_VERLLV = find_mi("VERLLV");
constexpr auto mi_VESL = find_mi("VESL");
constexpr auto mi_VESLV = find_mi("VESLV");
constexpr auto mi_VESRA = find_mi("VESRA");
constexpr auto mi_VESRAV = find_mi("VESRAV");
constexpr auto mi_VESRL = find_mi("VESRL");
constexpr auto mi_VESRLV = find_mi("VESRLV");
constexpr auto mi_VFA = find_mi("VFA");
constexpr auto mi_VFAE = find_mi("VFAE");
constexpr auto mi_VFCE = find_mi("VFCE");
constexpr auto mi_VFCH = find_mi("VFCH");
constexpr auto mi_VFCHE = find_mi("VFCHE");
constexpr auto mi_VFD = find_mi("VFD");
constexpr auto mi_VFEE = find_mi("VFEE");
constexpr auto mi_VFENE = find_mi("VFENE");
constexpr auto mi_VFI = find_mi("VFI");
constexpr auto mi_VFLL = find_mi("VFLL");
constexpr auto mi_VFLR = find_mi("VFLR");
constexpr auto mi_VFM = find_mi("VFM");
constexpr auto mi_VFMA = find_mi("VFMA");
constexpr auto mi_VFMAX = find_mi("VFMAX");
constexpr auto mi_VFMIN = find_mi("VFMIN");
constexpr auto mi_VFMS = find_mi("VFMS");
constexpr auto mi_VFNMA = find_mi("VFNMA");
constexpr auto mi_VFNMS = find_mi("VFNMS");
constexpr auto mi_VFPSO = find_mi("VFPSO");
constexpr auto mi_VFS = find_mi("VFS");
constexpr auto mi_VFSQ = find_mi("VFSQ");
constexpr auto mi_VFTCI = find_mi("VFTCI");
constexpr auto mi_VGBM = find_mi("VGBM");
constexpr auto mi_VGFM = find_mi("VGFM");
constexpr auto mi_VGFMA = find_mi("VGFMA");
constexpr auto mi_VGM = find_mi("VGM");
constexpr auto mi_VISTR = find_mi("VISTR");
constexpr auto mi_VLBR = find_mi("VLBR");
constexpr auto mi_VLBRREP = find_mi("VLBRREP");
constexpr auto mi_VLC = find_mi("VLC");
constexpr auto mi_VLER = find_mi("VLER");
constexpr auto mi_VLGV = find_mi("VLGV");
constexpr auto mi_VLLEBRZ = find_mi("VLLEBRZ");
constexpr auto mi_VLLEZ = find_mi("VLLEZ");
constexpr auto mi_VLP = find_mi("VLP");
constexpr auto mi_VLREP = find_mi("VLREP");
constexpr auto mi_VLVG = find_mi("VLVG");
constexpr auto mi_VMAE = find_mi("VMAE");
constexpr auto mi_VMAH = find_mi("VMAH");
constexpr auto mi_VMAL = find_mi("VMAL");
constexpr auto mi_VMALE = find_mi("VMALE");
constexpr auto mi_VMALH = find_mi("VMALH");
constexpr auto mi_VMALO = find_mi("VMALO");
constexpr auto mi_VMAO = find_mi("VMAO");
constexpr auto mi_VME = find_mi("VME");
constexpr auto mi_VMH = find_mi("VMH");
constexpr auto mi_VML = find_mi("VML");
constexpr auto mi_VMLE = find_mi("VMLE");
constexpr auto mi_VMLH = find_mi("VMLH");
constexpr auto mi_VMLO = find_mi("VMLO");
constexpr auto mi_VMN = find_mi("VMN");
constexpr auto mi_VMNL = find_mi("VMNL");
constexpr auto mi_VMO = find_mi("VMO");
constexpr auto mi_VMRH = find_mi("VMRH");
constexpr auto mi_VMRL = find_mi("VMRL");
constexpr auto mi_VMSL = find_mi("VMSL");
constexpr auto mi_VMX = find_mi("VMX");
constexpr auto mi_VMXL = find_mi("VMXL");
constexpr auto mi_VNO = find_mi("VNO");
constexpr auto mi_VPK = find_mi("VPK");
constexpr auto mi_VPKLS = find_mi("VPKLS");
constexpr auto mi_VPKS = find_mi("VPKS");
constexpr auto mi_VPOPCT = find_mi("VPOPCT");
constexpr auto mi_VREP = find_mi("VREP");
constexpr auto mi_VREPI = find_mi("VREPI");
constexpr auto mi_VS = find_mi("VS");
constexpr auto mi_VSBCBI = find_mi("VSBCBI");
constexpr auto mi_VSBI = find_mi("VSBI");
constexpr auto mi_VSCBI = find_mi("VSCBI");
constexpr auto mi_VSCHP = find_mi("VSCHP");
constexpr auto mi_VSEG = find_mi("VSEG");
constexpr auto mi_VSTBR = find_mi("VSTBR");
constexpr auto mi_VSTEBRF = find_mi("VSTEBRF");
constexpr auto mi_VSTEBRG = find_mi("VSTEBRG");
constexpr auto mi_VSTER = find_mi("VSTER");
constexpr auto mi_VSTRC = find_mi("VSTRC");
constexpr auto mi_VSTRS = find_mi("VSTRS");
constexpr auto mi_VSUM = find_mi("VSUM");
constexpr auto mi_VSUMG = find_mi("VSUMG");
constexpr auto mi_VSUMQ = find_mi("VSUMQ");
constexpr auto mi_VUPH = find_mi("VUPH");
constexpr auto mi_VUPL = find_mi("VUPL");
constexpr auto mi_VUPLH = find_mi("VUPLH");
constexpr auto mi_VUPLL = find_mi("VUPLL");
constexpr auto mi_WFC = find_mi("WFC");
constexpr auto mi_WFK = find_mi("WFK");

constexpr mnemonic_code mnemonic_codes[] = {
    { "B", mi_BC, { { 15 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BE", mi_BC, { { 8 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BER", mi_BCR, { { 8 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BH", mi_BC, { { 2 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BHR", mi_BCR, { { 2 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BI", mi_BIC, { { 15 } }, UNI_SINCE_Z14 },
    { "BIE", mi_BIC, { { 8 } }, UNI_SINCE_Z14 },
    { "BIH", mi_BIC, { { 2 } }, UNI_SINCE_Z14 },
    { "BIL", mi_BIC, { { 4 } }, UNI_SINCE_Z14 },
    { "BIM", mi_BIC, { { 4 } }, UNI_SINCE_Z14 },
    { "BINE", mi_BIC, { { 7 } }, UNI_SINCE_Z14 },
    { "BINH", mi_BIC, { { 13 } }, UNI_SINCE_Z14 },
    { "BINL", mi_BIC, { { 11 } }, UNI_SINCE_Z14 },
    { "BINM", mi_BIC, { { 11 } }, UNI_SINCE_Z14 },
    { "BINO", mi_BIC, { { 14 } }, UNI_SINCE_Z14 },
    { "BINP", mi_BIC, { { 13 } }, UNI_SINCE_Z14 },
    { "BINZ", mi_BIC, { { 7 } }, UNI_SINCE_Z14 },
    { "BIO", mi_BIC, { { 1 } }, UNI_SINCE_Z14 },
    { "BIP", mi_BIC, { { 2 } }, UNI_SINCE_Z14 },
    { "BIZ", mi_BIC, { { 8 } }, UNI_SINCE_Z14 },
    { "BL", mi_BC, { { 4 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BLR", mi_BCR, { { 4 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BM", mi_BC, { { 4 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BMR", mi_BCR, { { 4 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BNE", mi_BC, { { 7 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BNER", mi_BCR, { { 7 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BNH", mi_BC, { { 13 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BNHR", mi_BCR, { { 13 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BNL", mi_BC, { { 11 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BNLR", mi_BCR, { { 11 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BNM", mi_BC, { { 11 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BNMR", mi_BCR, { { 11 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BNO", mi_BC, { { 14 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BNOR", mi_BCR, { { 14 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BNP", mi_BC, { { 13 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BNPR", mi_BCR, { { 13 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BNZ", mi_BC, { { 7 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BNZR", mi_BCR, { { 7 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BO", mi_BC, { { 1 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BOR", mi_BCR, { { 1 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BP", mi_BC, { { 2 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BPR", mi_BCR, { { 2 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BR", mi_BCR, { { 15 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BRE", mi_BRC, { { 8 } }, UNI_ESA_SINCE_ZOP },
    { "BREL", mi_BRCL, { { 8 } }, UNI_SINCE_ZOP },
    { "BRH", mi_BRC, { { 2 } }, UNI_ESA_SINCE_ZOP },
    { "BRHL", mi_BRCL, { { 2 } }, UNI_SINCE_ZOP },
    { "BRL", mi_BRC, { { 4 } }, UNI_ESA_SINCE_ZOP },
    { "BRLL", mi_BRCL, { { 4 } }, UNI_SINCE_ZOP },
    { "BRM", mi_BRC, { { 4 } }, UNI_ESA_SINCE_ZOP },
    { "BRML", mi_BRCL, { { 4 } }, UNI_SINCE_ZOP },
    { "BRNE", mi_BRC, { { 7 } }, UNI_ESA_SINCE_ZOP },
    { "BRNEL", mi_BRCL, { { 7 } }, UNI_SINCE_ZOP },
    { "BRNH", mi_BRC, { { 13 } }, UNI_ESA_SINCE_ZOP },
    { "BRNHL", mi_BRCL, { { 13 } }, UNI_SINCE_ZOP },
    { "BRNL", mi_BRC, { { 11 } }, UNI_ESA_SINCE_ZOP },
    { "BRNLL", mi_BRCL, { { 11 } }, UNI_SINCE_ZOP },
    { "BRNM", mi_BRC, { { 11 } }, UNI_ESA_SINCE_ZOP },
    { "BRNML", mi_BRCL, { { 11 } }, UNI_SINCE_ZOP },
    { "BRNO", mi_BRC, { { 14 } }, UNI_ESA_SINCE_ZOP },
    { "BRNOL", mi_BRCL, { { 14 } }, UNI_SINCE_ZOP },
    { "BRNP", mi_BRC, { { 13 } }, UNI_ESA_SINCE_ZOP },
    { "BRNPL", mi_BRCL, { { 13 } }, UNI_SINCE_ZOP },
    { "BRNZ", mi_BRC, { { 7 } }, UNI_ESA_SINCE_ZOP },
    { "BRNZL", mi_BRCL, { { 7 } }, UNI_SINCE_ZOP },
    { "BRO", mi_BRC, { { 1 } }, UNI_ESA_SINCE_ZOP },
    { "BROL", mi_BRCL, { { 1 } }, UNI_SINCE_ZOP },
    { "BRP", mi_BRC, { { 2 } }, UNI_ESA_SINCE_ZOP },
    { "BRPL", mi_BRCL, { { 2 } }, UNI_SINCE_ZOP },
    { "BRU", mi_BRC, { { 15 } }, UNI_ESA_SINCE_ZOP },
    { "BRUL", mi_BRCL, { { 15 } }, UNI_SINCE_ZOP },
    { "BRZ", mi_BRC, { { 8 } }, UNI_ESA_SINCE_ZOP },
    { "BRZL", mi_BRCL, { { 8 } }, UNI_SINCE_ZOP },
    { "BZ", mi_BC, { { 8 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "BZR", mi_BCR, { { 8 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "CGIBE", mi_CGIB, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CGIBH", mi_CGIB, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CGIBL", mi_CGIB, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CGIBNE", mi_CGIB, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CGIBNH", mi_CGIB, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CGIBNL", mi_CGIB, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CGIJE", mi_CGIJ, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CGIJH", mi_CGIJ, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CGIJL", mi_CGIJ, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CGIJNE", mi_CGIJ, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CGIJNH", mi_CGIJ, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CGIJNL", mi_CGIJ, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CGITE", mi_CGIT, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CGITH", mi_CGIT, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CGITL", mi_CGIT, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CGITNE", mi_CGIT, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CGITNH", mi_CGIT, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CGITNL", mi_CGIT, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CGRBE", mi_CGRB, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CGRBH", mi_CGRB, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CGRBL", mi_CGRB, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CGRBNE", mi_CGRB, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CGRBNH", mi_CGRB, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CGRBNL", mi_CGRB, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CGRJE", mi_CGRJ, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CGRJH", mi_CGRJ, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CGRJL", mi_CGRJ, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CGRJNE", mi_CGRJ, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CGRJNH", mi_CGRJ, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CGRJNL", mi_CGRJ, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CGRTE", mi_CGRT, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CGRTH", mi_CGRT, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CGRTL", mi_CGRT, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CGRTNE", mi_CGRT, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CGRTNH", mi_CGRT, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CGRTNL", mi_CGRT, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CIBE", mi_CIB, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CIBH", mi_CIB, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CIBL", mi_CIB, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CIBNE", mi_CIB, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CIBNH", mi_CIB, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CIBNL", mi_CIB, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CIJE", mi_CIJ, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CIJH", mi_CIJ, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CIJL", mi_CIJ, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CIJNE", mi_CIJ, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CIJNH", mi_CIJ, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CIJNL", mi_CIJ, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CITE", mi_CIT, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CITH", mi_CIT, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CITL", mi_CIT, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CITNE", mi_CIT, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CITNH", mi_CIT, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CITNL", mi_CIT, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CLFITE", mi_CLFIT, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CLFITH", mi_CLFIT, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CLFITL", mi_CLFIT, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CLFITNE", mi_CLFIT, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CLFITNH", mi_CLFIT, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CLFITNL", mi_CLFIT, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CLGIBE", mi_CLGIB, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CLGIBH", mi_CLGIB, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CLGIBL", mi_CLGIB, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CLGIBNE", mi_CLGIB, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CLGIBNH", mi_CLGIB, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CLGIBNL", mi_CLGIB, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CLGIJE", mi_CLGIJ, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CLGIJH", mi_CLGIJ, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CLGIJL", mi_CLGIJ, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CLGIJNE", mi_CLGIJ, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CLGIJNH", mi_CLGIJ, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CLGIJNL", mi_CLGIJ, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CLGITE", mi_CLGIT, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CLGITH", mi_CLGIT, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CLGITL", mi_CLGIT, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CLGITNE", mi_CLGIT, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CLGITNH", mi_CLGIT, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CLGITNL", mi_CLGIT, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CLGRBE", mi_CLGRB, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CLGRBH", mi_CLGRB, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CLGRBL", mi_CLGRB, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CLGRBNE", mi_CLGRB, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CLGRBNH", mi_CLGRB, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CLGRBNL", mi_CLGRB, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CLGRJE", mi_CLGRJ, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CLGRJH", mi_CLGRJ, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CLGRJL", mi_CLGRJ, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CLGRJNE", mi_CLGRJ, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CLGRJNH", mi_CLGRJ, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CLGRJNL", mi_CLGRJ, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CLGRTE", mi_CLGRT, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CLGRTH", mi_CLGRT, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CLGRTL", mi_CLGRT, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CLGRTNE", mi_CLGRT, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CLGRTNH", mi_CLGRT, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CLGRTNL", mi_CLGRT, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CLGTE", mi_CLGT, { { 1, 8 } }, UNI_SINCE_Z12 },
    { "CLGTH", mi_CLGT, { { 1, 2 } }, UNI_SINCE_Z12 },
    { "CLGTL", mi_CLGT, { { 1, 4 } }, UNI_SINCE_Z12 },
    { "CLGTNE", mi_CLGT, { { 1, 6 } }, UNI_SINCE_Z12 },
    { "CLGTNH", mi_CLGT, { { 1, 12 } }, UNI_SINCE_Z12 },
    { "CLGTNL", mi_CLGT, { { 1, 10 } }, UNI_SINCE_Z12 },
    { "CLIBE", mi_CLIB, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CLIBH", mi_CLIB, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CLIBL", mi_CLIB, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CLIBNE", mi_CLIB, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CLIBNH", mi_CLIB, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CLIBNL", mi_CLIB, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CLIJE", mi_CLIJ, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CLIJH", mi_CLIJ, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CLIJL", mi_CLIJ, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CLIJNE", mi_CLIJ, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CLIJNH", mi_CLIJ, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CLIJNL", mi_CLIJ, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CLRBE", mi_CLRB, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CLRBH", mi_CLRB, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CLRBL", mi_CLRB, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CLRBNE", mi_CLRB, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CLRBNH", mi_CLRB, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CLRBNL", mi_CLRB, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CLRJE", mi_CLRJ, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CLRJH", mi_CLRJ, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CLRJL", mi_CLRJ, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CLRJNE", mi_CLRJ, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CLRJNH", mi_CLRJ, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CLRJNL", mi_CLRJ, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CLRTE", mi_CLRT, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CLRTH", mi_CLRT, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CLRTL", mi_CLRT, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CLRTNE", mi_CLRT, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CLRTNH", mi_CLRT, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CLRTNL", mi_CLRT, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CLTE", mi_CLT, { { 1, 8 } }, UNI_SINCE_Z12 },
    { "CLTH", mi_CLT, { { 1, 2 } }, UNI_SINCE_Z12 },
    { "CLTL", mi_CLT, { { 1, 4 } }, UNI_SINCE_Z12 },
    { "CLTNE", mi_CLT, { { 1, 6 } }, UNI_SINCE_Z12 },
    { "CLTNH", mi_CLT, { { 1, 12 } }, UNI_SINCE_Z12 },
    { "CLTNL", mi_CLT, { { 1, 10 } }, UNI_SINCE_Z12 },
    { "CRBE", mi_CRB, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CRBH", mi_CRB, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CRBL", mi_CRB, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CRBNE", mi_CRB, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CRBNH", mi_CRB, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CRBNL", mi_CRB, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CRJE", mi_CRJ, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CRJH", mi_CRJ, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CRJL", mi_CRJ, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CRJNE", mi_CRJ, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CRJNH", mi_CRJ, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CRJNL", mi_CRJ, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "CRTE", mi_CRT, { { 2, 8 } }, UNI_SINCE_Z10 },
    { "CRTH", mi_CRT, { { 2, 2 } }, UNI_SINCE_Z10 },
    { "CRTL", mi_CRT, { { 2, 4 } }, UNI_SINCE_Z10 },
    { "CRTNE", mi_CRT, { { 2, 6 } }, UNI_SINCE_Z10 },
    { "CRTNH", mi_CRT, { { 2, 12 } }, UNI_SINCE_Z10 },
    { "CRTNL", mi_CRT, { { 2, 10 } }, UNI_SINCE_Z10 },
    { "J", mi_BRC, { { 15 } }, UNI_ESA_SINCE_ZOP },
    { "JAS", mi_BRAS, {}, UNI_ESA_SINCE_ZOP },
    { "JASL", mi_BRASL, {}, UNI_ESA_SINCE_ZOP },
    { "JC", mi_BRC, {}, UNI_ESA_SINCE_ZOP },
    { "JCT", mi_BRCT, {}, UNI_ESA_SINCE_ZOP },
    { "JCTG", mi_BRCTG, {}, UNI_SINCE_ZOP },
    { "JCTH", mi_BRCTH, {}, UNI_SINCE_Z11 },
    { "JE", mi_BRC, { { 8 } }, UNI_ESA_SINCE_ZOP },
    { "JH", mi_BRC, { { 2 } }, UNI_ESA_SINCE_ZOP },
    { "JL", mi_BRC, { { 4 } }, UNI_ESA_SINCE_ZOP },
    { "JLC", mi_BRCL, {}, UNI_SINCE_ZOP },
    { "JLE", mi_BRCL, { { 8 } }, UNI_SINCE_ZOP },
    { "JLH", mi_BRCL, { { 2 } }, UNI_SINCE_ZOP },
    { "JLL", mi_BRCL, { { 4 } }, UNI_SINCE_ZOP },
    { "JLM", mi_BRCL, { { 4 } }, UNI_SINCE_ZOP },
    { "JLNE", mi_BRCL, { { 7 } }, UNI_SINCE_ZOP },
    { "JLNH", mi_BRCL, { { 13 } }, UNI_SINCE_ZOP },
    { "JLNL", mi_BRCL, { { 11 } }, UNI_SINCE_ZOP },
    { "JLNM", mi_BRCL, { { 11 } }, UNI_SINCE_ZOP },
    { "JLNO", mi_BRCL, { { 14 } }, UNI_SINCE_ZOP },
    { "JLNOP", mi_BRCL, { { 0 } }, UNI_ESA_SINCE_ZOP },
    { "JLNP", mi_BRCL, { { 13 } }, UNI_SINCE_ZOP },
    { "JLNZ", mi_BRCL, { { 7 } }, UNI_SINCE_ZOP },
    { "JLO", mi_BRCL, { { 1 } }, UNI_SINCE_ZOP },
    { "JLP", mi_BRCL, { { 2 } }, UNI_SINCE_ZOP },
    { "JLU", mi_BRCL, { { 15 } }, UNI_SINCE_ZOP },
    { "JLZ", mi_BRCL, { { 8 } }, UNI_SINCE_ZOP },
    { "JM", mi_BRC, { { 4 } }, UNI_ESA_SINCE_ZOP },
    { "JNE", mi_BRC, { { 7 } }, UNI_ESA_SINCE_ZOP },
    { "JNH", mi_BRC, { { 13 } }, UNI_ESA_SINCE_ZOP },
    { "JNL", mi_BRC, { { 11 } }, UNI_ESA_SINCE_ZOP },
    { "JNM", mi_BRC, { { 11 } }, UNI_ESA_SINCE_ZOP },
    { "JNO", mi_BRC, { { 14 } }, UNI_ESA_SINCE_ZOP },
    { "JNOP", mi_BRC, { { 0 } }, UNI_ESA_SINCE_ZOP },
    { "JNP", mi_BRC, { { 13 } }, UNI_ESA_SINCE_ZOP },
    { "JNZ", mi_BRC, { { 7 } }, UNI_ESA_SINCE_ZOP },
    { "JO", mi_BRC, { { 1 } }, UNI_ESA_SINCE_ZOP },
    { "JP", mi_BRC, { { 2 } }, UNI_ESA_SINCE_ZOP },
    { "JXH", mi_BRXH, {}, UNI_ESA_SINCE_ZOP },
    { "JXHG", mi_BRXHG, {}, UNI_SINCE_ZOP },
    { "JXLE", mi_BRXLE, {}, UNI_ESA_SINCE_ZOP },
    { "JXLEG", mi_BRXLG, {}, UNI_SINCE_ZOP },
    { "JZ", mi_BRC, { { 8 } }, UNI_ESA_SINCE_ZOP },
    { "LDRV", mi_VLLEBRZ, { { 2, 3 } }, UNI_SINCE_Z15 },
    { "LERV", mi_VLLEBRZ, { { 2, 6 } }, UNI_SINCE_Z15 },
    { "LFI", mi_IILF, {}, UNI_SINCE_Z16 },
    { "LHHR", mi_RISBHG, { { 2, 0 }, { 31 } }, UNI_SINCE_Z11 },
    { "LHLR", mi_RISBHG, { { 2, 0 }, { 31 }, { 32 } }, UNI_SINCE_Z11 },
    { "LLCHHR", mi_RISBHG, { { 2, 24 }, { 0, 0x80 + 31 } }, UNI_SINCE_Z11 },
    { "LLCHLR", mi_RISBHG, { { 2, 24 }, { 0, 0x80 + 31 }, { 32 } }, UNI_SINCE_Z11 },
    { "LLCLHR", mi_RISBLG, { { 2, 24 }, { 0, 0x80 + 31 }, { 32 } }, UNI_SINCE_Z11 },
    { "LLGFI", mi_LLILF, {}, UNI_SINCE_Z16 },
    { "LLHFR", mi_RISBLG, { { 2, 0 }, { 31 }, { 32 } }, UNI_SINCE_Z11 },
    { "LLHHHR", mi_RISBHG, { { 2, 16 }, { 0, 0x80 + 31 } }, UNI_SINCE_Z11 },
    { "LLHHLR", mi_RISBHG, { { 2, 16 }, { 0, 0x80 + 31 }, { 32 } }, UNI_SINCE_Z11 },
    { "LLHLHR", mi_RISBLG, { { 2, 16 }, { 0, 0x80 + 31 }, { 32 } }, UNI_SINCE_Z11 },
    { "LOCE", mi_LOC, { { 2, 8 } }, UNI_SINCE_Z11 },
    { "LOCFHE", mi_LOCFH, { { 2, 8 } }, UNI_SINCE_Z13 },
    { "LOCFHH", mi_LOCFH, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "LOCFHL", mi_LOCFH, { { 2, 4 } }, UNI_SINCE_Z13 },
    { "LOCFHM", mi_LOCFH, { { 2, 4 } }, UNI_SINCE_Z13 },
    { "LOCFHNE", mi_LOCFH, { { 2, 7 } }, UNI_SINCE_Z13 },
    { "LOCFHNH", mi_LOCFH, { { 2, 13 } }, UNI_SINCE_Z13 },
    { "LOCFHNL", mi_LOCFH, { { 2, 11 } }, UNI_SINCE_Z13 },
    { "LOCFHNM", mi_LOCFH, { { 2, 11 } }, UNI_SINCE_Z13 },
    { "LOCFHNO", mi_LOCFH, { { 2, 14 } }, UNI_SINCE_Z13 },
    { "LOCFHNP", mi_LOCFH, { { 2, 13 } }, UNI_SINCE_Z13 },
    { "LOCFHNZ", mi_LOCFH, { { 2, 7 } }, UNI_SINCE_Z13 },
    { "LOCFHO", mi_LOCFH, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "LOCFHP", mi_LOCFH, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "LOCFHRE", mi_LOCFHR, { { 2, 8 } }, UNI_SINCE_Z13 },
    { "LOCFHRH", mi_LOCFHR, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "LOCFHRL", mi_LOCFHR, { { 2, 4 } }, UNI_SINCE_Z13 },
    { "LOCFHRM", mi_LOCFHR, { { 2, 4 } }, UNI_SINCE_Z13 },
    { "LOCFHRNE", mi_LOCFHR, { { 2, 7 } }, UNI_SINCE_Z13 },
    { "LOCFHRNH", mi_LOCFHR, { { 2, 13 } }, UNI_SINCE_Z13 },
    { "LOCFHRNL", mi_LOCFHR, { { 2, 11 } }, UNI_SINCE_Z13 },
    { "LOCFHRNM", mi_LOCFHR, { { 2, 11 } }, UNI_SINCE_Z13 },
    { "LOCFHRNO", mi_LOCFHR, { { 2, 14 } }, UNI_SINCE_Z13 },
    { "LOCFHRNP", mi_LOCFHR, { { 2, 13 } }, UNI_SINCE_Z13 },
    { "LOCFHRNZ", mi_LOCFHR, { { 2, 7 } }, UNI_SINCE_Z13 },
    { "LOCFHRO", mi_LOCFHR, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "LOCFHRP", mi_LOCFHR, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "LOCFHRZ", mi_LOCFHR, { { 2, 8 } }, UNI_SINCE_Z13 },
    { "LOCFHZ", mi_LOCFH, { { 2, 8 } }, UNI_SINCE_Z13 },
    { "LOCGE", mi_LOCG, { { 2, 8 } }, UNI_SINCE_Z11 },
    { "LOCGH", mi_LOCG, { { 2, 2 } }, UNI_SINCE_Z11 },
    { "LOCGHIE", mi_LOCGHI, { { 2, 8 } }, UNI_SINCE_Z13 },
    { "LOCGHIH", mi_LOCGHI, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "LOCGHIL", mi_LOCGHI, { { 2, 4 } }, UNI_SINCE_Z13 },
    { "LOCGHIM", mi_LOCGHI, { { 2, 4 } }, UNI_SINCE_Z13 },
    { "LOCGHINE", mi_LOCGHI, { { 2, 7 } }, UNI_SINCE_Z13 },
    { "LOCGHINH", mi_LOCGHI, { { 2, 13 } }, UNI_SINCE_Z13 },
    { "LOCGHINL", mi_LOCGHI, { { 2, 11 } }, UNI_SINCE_Z13 },
    { "LOCGHINM", mi_LOCGHI, { { 2, 11 } }, UNI_SINCE_Z13 },
    { "LOCGHINO", mi_LOCGHI, { { 2, 14 } }, UNI_SINCE_Z13 },
    { "LOCGHINP", mi_LOCGHI, { { 2, 13 } }, UNI_SINCE_Z13 },
    { "LOCGHINZ", mi_LOCGHI, { { 2, 7 } }, UNI_SINCE_Z13 },
    { "LOCGHIO", mi_LOCGHI, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "LOCGHIP", mi_LOCGHI, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "LOCGHIZ", mi_LOCGHI, { { 2, 8 } }, UNI_SINCE_Z13 },
    { "LOCGL", mi_LOCG, { { 2, 4 } }, UNI_SINCE_Z11 },
    { "LOCGM", mi_LOCG, { { 2, 4 } }, UNI_SINCE_Z13 },
    { "LOCGNE", mi_LOCG, { { 2, 6 } }, UNI_SINCE_Z11 },
    { "LOCGNH", mi_LOCG, { { 2, 12 } }, UNI_SINCE_Z11 },
    { "LOCGNL", mi_LOCG, { { 2, 10 } }, UNI_SINCE_Z11 },
    { "LOCGNM", mi_LOCG, { { 2, 10 } }, UNI_SINCE_Z13 },
    { "LOCGNO", mi_LOCG, { { 2, 14 } }, UNI_SINCE_Z13 },
    { "LOCGNP", mi_LOCG, { { 2, 12 } }, UNI_SINCE_Z13 },
    { "LOCGNZ", mi_LOCG, { { 2, 6 } }, UNI_SINCE_Z13 },
    { "LOCGO", mi_LOCG, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "LOCGP", mi_LOCG, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "LOCGRE", mi_LOCGR, { { 2, 8 } }, UNI_SINCE_Z11 },
    { "LOCGRH", mi_LOCGR, { { 2, 2 } }, UNI_SINCE_Z11 },
    { "LOCGRL", mi_LOCGR, { { 2, 4 } }, UNI_SINCE_Z11 },
    { "LOCGRM", mi_LOCGR, { { 2, 4 } }, UNI_SINCE_Z13 },
    { "LOCGRNE", mi_LOCGR, { { 2, 6 } }, UNI_SINCE_Z11 },
    { "LOCGRNH", mi_LOCGR, { { 2, 12 } }, UNI_SINCE_Z11 },
    { "LOCGRNL", mi_LOCGR, { { 2, 10 } }, UNI_SINCE_Z11 },
    { "LOCGRNM", mi_LOCGR, { { 2, 10 } }, UNI_SINCE_Z13 },
    { "LOCGRNO", mi_LOCGR, { { 2, 14 } }, UNI_SINCE_Z13 },
    { "LOCGRNP", mi_LOCGR, { { 2, 12 } }, UNI_SINCE_Z13 },
    { "LOCGRNZ", mi_LOCGR, { { 2, 6 } }, UNI_SINCE_Z13 },
    { "LOCGRO", mi_LOCGR, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "LOCGRP", mi_LOCGR, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "LOCGRZ", mi_LOCGR, { { 2, 8 } }, UNI_SINCE_Z13 },
    { "LOCGZ", mi_LOCG, { { 2, 8 } }, UNI_SINCE_Z13 },
    { "LOCH", mi_LOC, { { 2, 2 } }, UNI_SINCE_Z11 },
    { "LOCHHIE", mi_LOCHHI, { { 2, 8 } }, UNI_SINCE_Z13 },
    { "LOCHHIH", mi_LOCHHI, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "LOCHHIL", mi_LOCHHI, { { 2, 4 } }, UNI_SINCE_Z13 },
    { "LOCHHIM", mi_LOCHHI, { { 2, 4 } }, UNI_SINCE_Z13 },
    { "LOCHHINE", mi_LOCHHI, { { 2, 7 } }, UNI_SINCE_Z13 },
    { "LOCHHINH", mi_LOCHHI, { { 2, 13 } }, UNI_SINCE_Z13 },
    { "LOCHHINL", mi_LOCHHI, { { 2, 11 } }, UNI_SINCE_Z13 },
    { "LOCHHINM", mi_LOCHHI, { { 2, 11 } }, UNI_SINCE_Z13 },
    { "LOCHHINO", mi_LOCHHI, { { 2, 14 } }, UNI_SINCE_Z13 },
    { "LOCHHINP", mi_LOCHHI, { { 2, 13 } }, UNI_SINCE_Z13 },
    { "LOCHHINZ", mi_LOCHHI, { { 2, 7 } }, UNI_SINCE_Z13 },
    { "LOCHHIO", mi_LOCHHI, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "LOCHHIP", mi_LOCHHI, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "LOCHHIZ", mi_LOCHHI, { { 2, 8 } }, UNI_SINCE_Z13 },
    { "LOCHIE", mi_LOCHI, { { 2, 8 } }, UNI_SINCE_Z13 },
    { "LOCHIH", mi_LOCHI, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "LOCHIL", mi_LOCHI, { { 2, 4 } }, UNI_SINCE_Z13 },
    { "LOCHIM", mi_LOCHI, { { 2, 4 } }, UNI_SINCE_Z13 },
    { "LOCHINE", mi_LOCHI, { { 2, 7 } }, UNI_SINCE_Z13 },
    { "LOCHINH", mi_LOCHI, { { 2, 13 } }, UNI_SINCE_Z13 },
    { "LOCHINL", mi_LOCHI, { { 2, 11 } }, UNI_SINCE_Z13 },
    { "LOCHINM", mi_LOCHI, { { 2, 11 } }, UNI_SINCE_Z13 },
    { "LOCHINO", mi_LOCHI, { { 2, 14 } }, UNI_SINCE_Z13 },
    { "LOCHINP", mi_LOCHI, { { 2, 13 } }, UNI_SINCE_Z13 },
    { "LOCHINZ", mi_LOCHI, { { 2, 7 } }, UNI_SINCE_Z13 },
    { "LOCHIO", mi_LOCHI, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "LOCHIP", mi_LOCHI, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "LOCHIZ", mi_LOCHI, { { 2, 8 } }, UNI_SINCE_Z13 },
    { "LOCL", mi_LOC, { { 2, 4 } }, UNI_SINCE_Z11 },
    { "LOCM", mi_LOC, { { 2, 4 } }, UNI_SINCE_Z13 },
    { "LOCNE", mi_LOC, { { 2, 6 } }, UNI_SINCE_Z11 },
    { "LOCNH", mi_LOC, { { 2, 12 } }, UNI_SINCE_Z11 },
    { "LOCNL", mi_LOC, { { 2, 10 } }, UNI_SINCE_Z11 },
    { "LOCNM", mi_LOC, { { 2, 10 } }, UNI_SINCE_Z13 },
    { "LOCNO", mi_LOC, { { 2, 14 } }, UNI_SINCE_Z13 },
    { "LOCNP", mi_LOC, { { 2, 12 } }, UNI_SINCE_Z13 },
    { "LOCNZ", mi_LOC, { { 2, 6 } }, UNI_SINCE_Z13 },
    { "LOCO", mi_LOC, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "LOCP", mi_LOC, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "LOCRE", mi_LOCR, { { 2, 8 } }, UNI_SINCE_Z11 },
    { "LOCRH", mi_LOCR, { { 2, 2 } }, UNI_SINCE_Z11 },
    { "LOCRL", mi_LOCR, { { 2, 4 } }, UNI_SINCE_Z11 },
    { "LOCRM", mi_LOCR, { { 2, 4 } }, UNI_SINCE_Z13 },
    { "LOCRNE", mi_LOCR, { { 2, 6 } }, UNI_SINCE_Z11 },
    { "LOCRNH", mi_LOCR, { { 2, 12 } }, UNI_SINCE_Z11 },
    { "LOCRNL", mi_LOCR, { { 2, 10 } }, UNI_SINCE_Z11 },
    { "LOCRNM", mi_LOCR, { { 2, 10 } }, UNI_SINCE_Z13 },
    { "LOCRNO", mi_LOCR, { { 2, 14 } }, UNI_SINCE_Z13 },
    { "LOCRNP", mi_LOCR, { { 2, 12 } }, UNI_SINCE_Z13 },
    { "LOCRNZ", mi_LOCR, { { 2, 6 } }, UNI_SINCE_Z13 },
    { "LOCRO", mi_LOCR, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "LOCRP", mi_LOCR, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "LOCRZ", mi_LOCR, { { 2, 8 } }, UNI_SINCE_Z13 },
    { "LOCZ", mi_LOC, { { 2, 8 } }, UNI_SINCE_Z13 },
    { "NHHR", mi_RNSBG, { { 2, 0 }, { 31 } }, UNI_SINCE_Z11 },
    { "NHLR", mi_RNSBG, { { 2, 0 }, { 31 }, { 32 } }, UNI_SINCE_Z11 },
    { "NLHR", mi_RNSBG, { { 2, 32 }, { 63 }, { 32 } }, UNI_SINCE_Z11 },
    { "NOP", mi_BC, { { 0 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "NOPR", mi_BCR, { { 0 } }, UNI_ESA_XA_370_DOS_SINCE_ZOP },
    { "NOTGR", mi_NOGRK, { mnemonic_transformation(2, mnemonic_transformation_kind::copy, 1) }, UNI_SINCE_Z15 },
    { "NOTR", mi_NORK, { mnemonic_transformation(2, mnemonic_transformation_kind::copy, 1) }, UNI_SINCE_Z15 },
    { "OHHR", mi_ROSBG, { { 2, 0 }, { 31 } }, UNI_SINCE_Z11 },
    { "OHLR", mi_ROSBG, { { 2, 0 }, { 31 }, { 32 } }, UNI_SINCE_Z11 },
    { "OLHR", mi_ROSBG, { { 2, 32 }, { 63 }, { 32 } }, UNI_SINCE_Z11 },
    { "RISBGNZ",
        mi_RISBGN,
        { mnemonic_transformation(3, 0x80, mnemonic_transformation_kind::or_with, 3, false) },
        UNI_SINCE_Z12 },
    { "RISBGZ",
        mi_RISBG,
        { mnemonic_transformation(3, 0x80, mnemonic_transformation_kind::or_with, 3, false) },
        UNI_SINCE_Z10 },
    { "RISBHGZ",
        mi_RISBHG,
        { mnemonic_transformation(3, 0x80, mnemonic_transformation_kind::or_with, 3, false) },
        UNI_SINCE_Z11 },
    { "RISBLGZ",
        mi_RISBLG,
        { mnemonic_transformation(3, 0x80, mnemonic_transformation_kind::or_with, 3, false) },
        UNI_SINCE_Z11 },
    { "RNSBGT",
        mi_RNSBG,
        { mnemonic_transformation(2, 0x80, mnemonic_transformation_kind::or_with, 2, false) },
        UNI_SINCE_Z10 },
    { "ROSBGT",
        mi_ROSBG,
        { mnemonic_transformation(2, 0x80, mnemonic_transformation_kind::or_with, 2, false) },
        UNI_SINCE_Z10 },
    { "RXSBGT",
        mi_RXSBG,
        { mnemonic_transformation(2, 0x80, mnemonic_transformation_kind::or_with, 2, false) },
        UNI_SINCE_Z10 },
    { "SELFHRE", mi_SELFHR, { { 3, 8 } }, UNI_SINCE_Z15 },
    { "SELFHRH", mi_SELFHR, { { 3, 2 } }, UNI_SINCE_Z15 },
    { "SELFHRL", mi_SELFHR, { { 3, 4 } }, UNI_SINCE_Z15 },
    { "SELFHRM", mi_SELFHR, { { 3, 4 } }, UNI_SINCE_Z15 },
    { "SELFHRNE", mi_SELFHR, { { 3, 7 } }, UNI_SINCE_Z15 },
    { "SELFHRNH", mi_SELFHR, { { 3, 13 } }, UNI_SINCE_Z15 },
    { "SELFHRNL", mi_SELFHR, { { 3, 11 } }, UNI_SINCE_Z15 },
    { "SELFHRNM", mi_SELFHR, { { 3, 11 } }, UNI_SINCE_Z15 },
    { "SELFHRNO", mi_SELFHR, { { 3, 14 } }, UNI_SINCE_Z15 },
    { "SELFHRNP", mi_SELFHR, { { 3, 13 } }, UNI_SINCE_Z15 },
    { "SELFHRNZ", mi_SELFHR, { { 3, 7 } }, UNI_SINCE_Z15 },
    { "SELFHRO", mi_SELFHR, { { 3, 1 } }, UNI_SINCE_Z15 },
    { "SELFHRP", mi_SELFHR, { { 3, 2 } }, UNI_SINCE_Z15 },
    { "SELFHRZ", mi_SELFHR, { { 3, 8 } }, UNI_SINCE_Z15 },
    { "SELGRE", mi_SELGR, { { 3, 8 } }, UNI_SINCE_Z15 },
    { "SELGRH", mi_SELGR, { { 3, 2 } }, UNI_SINCE_Z15 },
    { "SELGRL", mi_SELGR, { { 3, 4 } }, UNI_SINCE_Z15 },
    { "SELGRM", mi_SELGR, { { 3, 4 } }, UNI_SINCE_Z15 },
    { "SELGRNE", mi_SELGR, { { 3, 7 } }, UNI_SINCE_Z15 },
    { "SELGRNH", mi_SELGR, { { 3, 13 } }, UNI_SINCE_Z15 },
    { "SELGRNL", mi_SELGR, { { 3, 11 } }, UNI_SINCE_Z15 },
    { "SELGRNM", mi_SELGR, { { 3, 11 } }, UNI_SINCE_Z15 },
    { "SELGRNO", mi_SELGR, { { 3, 14 } }, UNI_SINCE_Z15 },
    { "SELGRNP", mi_SELGR, { { 3, 13 } }, UNI_SINCE_Z15 },
    { "SELGRNZ", mi_SELGR, { { 3, 7 } }, UNI_SINCE_Z15 },
    { "SELGRO", mi_SELGR, { { 3, 1 } }, UNI_SINCE_Z15 },
    { "SELGRP", mi_SELGR, { { 3, 2 } }, UNI_SINCE_Z15 },
    { "SELGRZ", mi_SELGR, { { 3, 8 } }, UNI_SINCE_Z15 },
    { "SELRE", mi_SELR, { { 3, 8 } }, UNI_SINCE_Z15 },
    { "SELRH", mi_SELR, { { 3, 2 } }, UNI_SINCE_Z15 },
    { "SELRL", mi_SELR, { { 3, 4 } }, UNI_SINCE_Z15 },
    { "SELRM", mi_SELR, { { 3, 4 } }, UNI_SINCE_Z15 },
    { "SELRNE", mi_SELR, { { 3, 7 } }, UNI_SINCE_Z15 },
    { "SELRNH", mi_SELR, { { 3, 13 } }, UNI_SINCE_Z15 },
    { "SELRNL", mi_SELR, { { 3, 11 } }, UNI_SINCE_Z15 },
    { "SELRNM", mi_SELR, { { 3, 11 } }, UNI_SINCE_Z15 },
    { "SELRNO", mi_SELR, { { 3, 14 } }, UNI_SINCE_Z15 },
    { "SELRNP", mi_SELR, { { 3, 13 } }, UNI_SINCE_Z15 },
    { "SELRNZ", mi_SELR, { { 3, 7 } }, UNI_SINCE_Z15 },
    { "SELRO", mi_SELR, { { 3, 1 } }, UNI_SINCE_Z15 },
    { "SELRP", mi_SELR, { { 3, 2 } }, UNI_SINCE_Z15 },
    { "SELRZ", mi_SELR, { { 3, 8 } }, UNI_SINCE_Z15 },
    { "SLLHH",
        mi_RISBHG,
        {
            mnemonic_transformation(2, 0, false),
            mnemonic_transformation(0, 0x80 + 31, mnemonic_transformation_kind::subtract_from, 2),
            mnemonic_transformation(0, mnemonic_transformation_kind::copy, 2),
        },
        UNI_SINCE_Z16 },
    { "SLLHL",
        mi_RISBHG,
        {
            mnemonic_transformation(2, 0, false),
            mnemonic_transformation(0, 0x80 + 31, mnemonic_transformation_kind::subtract_from, 2),
            mnemonic_transformation(0, 32, mnemonic_transformation_kind::add_to, 2),
        },
        UNI_SINCE_Z16 },
    { "SLLLH",
        mi_RISBLG,
        {
            mnemonic_transformation(2, 0, false),
            mnemonic_transformation(0, 0x80 + 31, mnemonic_transformation_kind::subtract_from, 2),
            mnemonic_transformation(0, 32, mnemonic_transformation_kind::add_to, 2),
        },
        UNI_SINCE_Z16 },
    { "SRLHH",
        mi_RISBHG,
        {
            mnemonic_transformation(3, 0x80 + 31),
            mnemonic_transformation(0, 6, mnemonic_transformation_kind::complement, 2),
        },
        UNI_SINCE_Z16 },
    { "SRLHL",
        mi_RISBHG,
        {
            mnemonic_transformation(3, 0x80 + 31),
            mnemonic_transformation(0, 32, mnemonic_transformation_kind::subtract_from, 2),
        },
        UNI_SINCE_Z16 },
    { "SRLLH",
        mi_RISBLG,
        {
            mnemonic_transformation(3, 0x80 + 31),
            mnemonic_transformation(0, 32, mnemonic_transformation_kind::subtract_from, 2),
        },
        UNI_SINCE_Z16 },
    { "STDRV", mi_VSTEBRG, { { 2, 0 } }, UNI_SINCE_Z15 },
    { "STERV", mi_VSTEBRF, { { 2, 0 } }, UNI_SINCE_Z15 },
    { "STOCE", mi_STOC, { { 2, 8 } }, UNI_SINCE_Z11 },
    { "STOCFHE", mi_STOCFH, { { 2, 8 } }, UNI_SINCE_Z13 },
    { "STOCFHH", mi_STOCFH, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "STOCFHL", mi_STOCFH, { { 2, 4 } }, UNI_SINCE_Z13 },
    { "STOCFHM", mi_STOCFH, { { 2, 4 } }, UNI_SINCE_Z13 },
    { "STOCFHNE", mi_STOCFH, { { 2, 7 } }, UNI_SINCE_Z13 },
    { "STOCFHNH", mi_STOCFH, { { 2, 13 } }, UNI_SINCE_Z13 },
    { "STOCFHNL", mi_STOCFH, { { 2, 11 } }, UNI_SINCE_Z13 },
    { "STOCFHNM", mi_STOCFH, { { 2, 11 } }, UNI_SINCE_Z13 },
    { "STOCFHNO", mi_STOCFH, { { 2, 14 } }, UNI_SINCE_Z13 },
    { "STOCFHNP", mi_STOCFH, { { 2, 13 } }, UNI_SINCE_Z13 },
    { "STOCFHNZ", mi_STOCFH, { { 2, 7 } }, UNI_SINCE_Z13 },
    { "STOCFHO", mi_STOCFH, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "STOCFHP", mi_STOCFH, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "STOCFHZ", mi_STOCFH, { { 2, 8 } }, UNI_SINCE_Z13 },
    { "STOCGE", mi_STOCG, { { 2, 8 } }, UNI_SINCE_Z11 },
    { "STOCGH", mi_STOCG, { { 2, 2 } }, UNI_SINCE_Z11 },
    { "STOCGL", mi_STOCG, { { 2, 4 } }, UNI_SINCE_Z11 },
    { "STOCGM", mi_STOCG, { { 2, 4 } }, UNI_SINCE_Z13 },
    { "STOCGNE", mi_STOCG, { { 2, 6 } }, UNI_SINCE_Z11 },
    { "STOCGNH", mi_STOCG, { { 2, 12 } }, UNI_SINCE_Z11 },
    { "STOCGNL", mi_STOCG, { { 2, 10 } }, UNI_SINCE_Z11 },
    { "STOCGNM", mi_STOCG, { { 2, 10 } }, UNI_SINCE_Z13 },
    { "STOCGNO", mi_STOCG, { { 2, 14 } }, UNI_SINCE_Z13 },
    { "STOCGNP", mi_STOCG, { { 2, 12 } }, UNI_SINCE_Z13 },
    { "STOCGNZ", mi_STOCG, { { 2, 6 } }, UNI_SINCE_Z13 },
    { "STOCGO", mi_STOCG, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "STOCGP", mi_STOCG, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "STOCGZ", mi_STOCG, { { 2, 8 } }, UNI_SINCE_Z13 },
    { "STOCH", mi_STOC, { { 2, 2 } }, UNI_SINCE_Z11 },
    { "STOCL", mi_STOC, { { 2, 4 } }, UNI_SINCE_Z11 },
    { "STOCM", mi_STOC, { { 2, 4 } }, UNI_SINCE_Z13 },
    { "STOCNE", mi_STOC, { { 2, 6 } }, UNI_SINCE_Z11 },
    { "STOCNH", mi_STOC, { { 2, 12 } }, UNI_SINCE_Z11 },
    { "STOCNL", mi_STOC, { { 2, 10 } }, UNI_SINCE_Z11 },
    { "STOCNM", mi_STOC, { { 2, 10 } }, UNI_SINCE_Z13 },
    { "STOCNO", mi_STOC, { { 2, 14 } }, UNI_SINCE_Z13 },
    { "STOCNP", mi_STOC, { { 2, 12 } }, UNI_SINCE_Z13 },
    { "STOCNZ", mi_STOC, { { 2, 6 } }, UNI_SINCE_Z13 },
    { "STOCO", mi_STOC, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "STOCP", mi_STOC, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "STOCZ", mi_STOC, { { 2, 8 } }, UNI_SINCE_Z13 },
    { "VAB", mi_VA, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VACCB", mi_VACC, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VACCCQ", mi_VACCC, { { 3, 4 } }, UNI_SINCE_Z13 },
    { "VACCF", mi_VACC, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VACCG", mi_VACC, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VACCH", mi_VACC, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VACCQ", mi_VACC, { { 3, 4 } }, UNI_SINCE_Z13 },
    { "VACQ", mi_VAC, { { 3, 4 } }, UNI_SINCE_Z13 },
    { "VAF", mi_VA, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VAG", mi_VA, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VAH", mi_VA, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VAQ", mi_VA, { { 3, 4 } }, UNI_ESA_XA_370_SINCE_Z13 },
    { "VAVGB", mi_VAVG, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VAVGF", mi_VAVG, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VAVGG", mi_VAVG, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VAVGH", mi_VAVG, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VAVGLB", mi_VAVGL, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VAVGLF", mi_VAVGL, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VAVGLG", mi_VAVGL, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VAVGLH", mi_VAVGL, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VCDG", mi_VCFPS, {}, UNI_SINCE_Z13 },
    { "VCDGB", mi_VCFPS, { { 2, 3 } }, UNI_SINCE_Z13 },
    { "VCDLG", mi_VCFPL, {}, UNI_SINCE_Z13 },
    { "VCDLGB", mi_VCFPL, { { 2, 3 } }, UNI_SINCE_Z13 },
    { "VCEFB", mi_VCFPS, { { 2, 0 } }, UNI_SINCE_Z15 },
    { "VCELFB", mi_VCFPL, { { 2, 0 } }, UNI_SINCE_Z15 },
    { "VCEQB", mi_VCEQ, { { 3, 0 }, { 0 } }, UNI_SINCE_Z13 },
    { "VCEQBS", mi_VCEQ, { { 3, 0 }, { 1 } }, UNI_SINCE_Z13 },
    { "VCEQF", mi_VCEQ, { { 3, 2 }, { 0 } }, UNI_SINCE_Z13 },
    { "VCEQFS", mi_VCEQ, { { 3, 2 }, { 1 } }, UNI_SINCE_Z13 },
    { "VCEQG", mi_VCEQ, { { 3, 3 }, { 0 } }, UNI_SINCE_Z13 },
    { "VCEQGS", mi_VCEQ, { { 3, 3 }, { 1 } }, UNI_SINCE_Z13 },
    { "VCEQH", mi_VCEQ, { { 3, 1 }, { 0 } }, UNI_SINCE_Z13 },
    { "VCEQHS", mi_VCEQ, { { 3, 1 }, { 1 } }, UNI_SINCE_Z13 },
    { "VCFEB", mi_VCSFP, { { 2, 2 } }, UNI_SINCE_Z15 },
    { "VCGD", mi_VCSFP, {}, UNI_SINCE_Z13 },
    { "VCGDB", mi_VCSFP, { { 2, 3 } }, UNI_SINCE_Z13 },
    { "VCHB", mi_VCH, { { 3, 0 }, { 0 } }, UNI_SINCE_Z13 },
    { "VCHBS", mi_VCH, { { 3, 0 }, { 1 } }, UNI_SINCE_Z13 },
    { "VCHF", mi_VCH, { { 3, 2 }, { 0 } }, UNI_SINCE_Z13 },
    { "VCHFS", mi_VCH, { { 3, 2 }, { 1 } }, UNI_SINCE_Z13 },
    { "VCHG", mi_VCH, { { 3, 3 }, { 0 } }, UNI_SINCE_Z13 },
    { "VCHGS", mi_VCH, { { 3, 3 }, { 1 } }, UNI_SINCE_Z13 },
    { "VCHH", mi_VCH, { { 3, 1 }, { 0 } }, UNI_SINCE_Z13 },
    { "VCHHS", mi_VCH, { { 3, 1 }, { 1 } }, UNI_SINCE_Z13 },
    { "VCHLB", mi_VCHL, { { 3, 0 }, { 0 } }, UNI_SINCE_Z13 },
    { "VCHLBS", mi_VCHL, { { 3, 0 }, { 1 } }, UNI_SINCE_Z13 },
    { "VCHLF", mi_VCHL, { { 3, 2 }, { 0 } }, UNI_SINCE_Z13 },
    { "VCHLFS", mi_VCHL, { { 3, 2 }, { 1 } }, UNI_SINCE_Z13 },
    { "VCHLG", mi_VCHL, { { 3, 3 }, { 0 } }, UNI_SINCE_Z13 },
    { "VCHLGS", mi_VCHL, { { 3, 3 }, { 1 } }, UNI_SINCE_Z13 },
    { "VCHLH", mi_VCHL, { { 3, 1 }, { 0 } }, UNI_SINCE_Z13 },
    { "VCHLHS", mi_VCHL, { { 3, 1 }, { 1 } }, UNI_SINCE_Z13 },
    { "VCLFEB", mi_VCLFP, { { 2, 0 } }, UNI_SINCE_Z15 },
    { "VCLGD", mi_VCLFP, {}, UNI_SINCE_Z13 },
    { "VCLGDB", mi_VCLFP, { { 2, 3 } }, UNI_SINCE_Z13 },
    { "VCLZB", mi_VCLZ, { { 2, 0 } }, UNI_SINCE_Z13 },
    { "VCLZF", mi_VCLZ, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "VCLZG", mi_VCLZ, { { 2, 3 } }, UNI_SINCE_Z13 },
    { "VCLZH", mi_VCLZ, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "VCTZB", mi_VCTZ, { { 2, 0 } }, UNI_SINCE_Z13 },
    { "VCTZF", mi_VCTZ, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "VCTZG", mi_VCTZ, { { 2, 3 } }, UNI_SINCE_Z13 },
    { "VCTZH", mi_VCTZ, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "VECB", mi_VEC, { { 2, 0 } }, UNI_SINCE_Z13 },
    { "VECF", mi_VEC, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "VECG", mi_VEC, { { 2, 3 } }, UNI_SINCE_Z13 },
    { "VECH", mi_VEC, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "VECLB", mi_VECL, { { 2, 0 } }, UNI_SINCE_Z13 },
    { "VECLF", mi_VECL, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "VECLG", mi_VECL, { { 2, 3 } }, UNI_SINCE_Z13 },
    { "VECLH", mi_VECL, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "VERIMB", mi_VERIM, { { 4, 0 } }, UNI_SINCE_Z13 },
    { "VERIMF", mi_VERIM, { { 4, 2 } }, UNI_SINCE_Z13 },
    { "VERIMG", mi_VERIM, { { 4, 3 } }, UNI_SINCE_Z13 },
    { "VERIMH", mi_VERIM, { { 4, 1 } }, UNI_SINCE_Z13 },
    { "VERLLB", mi_VERLL, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VERLLF", mi_VERLL, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VERLLG", mi_VERLL, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VERLLH", mi_VERLL, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VERLLVB", mi_VERLLV, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VERLLVF", mi_VERLLV, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VERLLVG", mi_VERLLV, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VERLLVH", mi_VERLLV, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VESLB", mi_VESL, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VESLF", mi_VESL, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VESLG", mi_VESL, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VESLH", mi_VESL, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VESLVB", mi_VESLV, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VESLVF", mi_VESLV, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VESLVG", mi_VESLV, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VESLVH", mi_VESLV, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VESRAB", mi_VESRA, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VESRAF", mi_VESRA, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VESRAG", mi_VESRA, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VESRAH", mi_VESRA, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VESRAVB", mi_VESRAV, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VESRAVF", mi_VESRAV, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VESRAVG", mi_VESRAV, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VESRAVH", mi_VESRAV, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VESRLB", mi_VESRL, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VESRLF", mi_VESRL, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VESRLG", mi_VESRL, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VESRLH", mi_VESRL, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VESRLVB", mi_VESRLV, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VESRLVF", mi_VESRLV, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VESRLVG", mi_VESRLV, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VESRLVH", mi_VESRLV, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VFADB", mi_VFA, { { 3, 3 }, { 0 } }, UNI_SINCE_Z13 },
    { "VFAEB", mi_VFAE, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VFAEBS",
        mi_VFAE,
        { { 3, 0 }, mnemonic_transformation(0, 1, mnemonic_transformation_kind::or_with, 3, false) },
        UNI_SINCE_Z13 },
    { "VFAEF", mi_VFAE, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VFAEFS",
        mi_VFAE,
        { { 3, 2 }, mnemonic_transformation(0, 1, mnemonic_transformation_kind::or_with, 3, false) },
        UNI_SINCE_Z13 },
    { "VFAEH", mi_VFAE, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VFAEHS",
        mi_VFAE,
        { { 3, 1 }, mnemonic_transformation(0, 1, mnemonic_transformation_kind::or_with, 3, false) },
        UNI_SINCE_Z13 },
    { "VFAEZB",
        mi_VFAE,
        { { 3, 0 }, mnemonic_transformation(0, 2, mnemonic_transformation_kind::or_with, 3, false) },
        UNI_SINCE_Z13 },
    { "VFAEZBS",
        mi_VFAE,
        { { 3, 0 }, mnemonic_transformation(0, 3, mnemonic_transformation_kind::or_with, 3, false) },
        UNI_SINCE_Z13 },
    { "VFAEZF",
        mi_VFAE,
        { { 3, 2 }, mnemonic_transformation(0, 2, mnemonic_transformation_kind::or_with, 3, false) },
        UNI_SINCE_Z13 },
    { "VFAEZFS",
        mi_VFAE,
        { { 3, 2 }, mnemonic_transformation(0, 3, mnemonic_transformation_kind::or_with, 3, false) },
        UNI_SINCE_Z13 },
    { "VFAEZH",
        mi_VFAE,
        { { 3, 1 }, mnemonic_transformation(0, 2, mnemonic_transformation_kind::or_with, 3, false) },
        UNI_SINCE_Z13 },
    { "VFAEZHS",
        mi_VFAE,
        { { 3, 1 }, mnemonic_transformation(0, 3, mnemonic_transformation_kind::or_with, 3, false) },
        UNI_SINCE_Z13 },
    { "VFASB", mi_VFA, { { 3, 2 }, { 0 } }, UNI_SINCE_Z14 },
    { "VFCEDB", mi_VFCE, { { 3, 3 }, { 0 }, { 0 } }, UNI_SINCE_Z13 },
    { "VFCEDBS", mi_VFCE, { { 3, 3 }, { 0 }, { 1 } }, UNI_SINCE_Z13 },
    { "VFCESB", mi_VFCE, { { 3, 2 }, { 0 }, { 0 } }, UNI_SINCE_Z14 },
    { "VFCESBS", mi_VFCE, { { 3, 2 }, { 0 }, { 1 } }, UNI_SINCE_Z14 },
    { "VFCHDB", mi_VFCH, { { 3, 3 }, { 0 }, { 0 } }, UNI_SINCE_Z13 },
    { "VFCHDBS", mi_VFCH, { { 3, 3 }, { 0 }, { 1 } }, UNI_SINCE_Z13 },
    { "VFCHEDB", mi_VFCHE, { { 3, 3 }, { 0 }, { 0 } }, UNI_SINCE_Z13 },
    { "VFCHEDBS", mi_VFCHE, { { 3, 3 }, { 0 }, { 1 } }, UNI_SINCE_Z13 },
    { "VFCHESB", mi_VFCHE, { { 3, 2 }, { 0 }, { 0 } }, UNI_SINCE_Z14 },
    { "VFCHESBS", mi_VFCHE, { { 3, 2 }, { 0 }, { 1 } }, UNI_SINCE_Z14 },
    { "VFCHSB", mi_VFCH, { { 3, 2 }, { 0 }, { 0 } }, UNI_SINCE_Z14 },
    { "VFCHSBS", mi_VFCH, { { 3, 2 }, { 0 }, { 1 } }, UNI_SINCE_Z14 },
    { "VFDDB", mi_VFD, { { 3, 3 }, { 0 } }, UNI_SINCE_Z13 },
    { "VFDSB", mi_VFD, { { 3, 2 }, { 0 } }, UNI_SINCE_Z14 },
    { "VFEEB", mi_VFEE, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VFEEBS", mi_VFEE, { { 3, 0 }, { 1 } }, UNI_SINCE_Z13 },
    { "VFEEF", mi_VFEE, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VFEEFS", mi_VFEE, { { 3, 2 }, { 1 } }, UNI_SINCE_Z13 },
    { "VFEEH", mi_VFEE, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VFEEHS", mi_VFEE, { { 3, 1 }, { 1 } }, UNI_SINCE_Z13 },
    { "VFEEZB", mi_VFEE, { { 3, 0 }, { 2 } }, UNI_SINCE_Z13 },
    { "VFEEZBS", mi_VFEE, { { 3, 0 }, { 3 } }, UNI_SINCE_Z13 },
    { "VFEEZF", mi_VFEE, { { 3, 2 }, { 2 } }, UNI_SINCE_Z13 },
    { "VFEEZFS", mi_VFEE, { { 3, 2 }, { 3 } }, UNI_SINCE_Z13 },
    { "VFEEZH", mi_VFEE, { { 3, 1 }, { 2 } }, UNI_SINCE_Z13 },
    { "VFEEZHS", mi_VFEE, { { 3, 1 }, { 3 } }, UNI_SINCE_Z13 },
    { "VFENEB", mi_VFENE, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VFENEBS", mi_VFENE, { { 3, 0 }, { 1 } }, UNI_SINCE_Z13 },
    { "VFENEF", mi_VFENE, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VFENEFS", mi_VFENE, { { 3, 2 }, { 1 } }, UNI_SINCE_Z13 },
    { "VFENEH", mi_VFENE, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VFENEHS", mi_VFENE, { { 3, 1 }, { 1 } }, UNI_SINCE_Z13 },
    { "VFENEZB", mi_VFENE, { { 3, 0 }, { 2 } }, UNI_SINCE_Z13 },
    { "VFENEZBS", mi_VFENE, { { 3, 0 }, { 3 } }, UNI_SINCE_Z13 },
    { "VFENEZF", mi_VFENE, { { 3, 2 }, { 2 } }, UNI_SINCE_Z13 },
    { "VFENEZFS", mi_VFENE, { { 3, 2 }, { 3 } }, UNI_SINCE_Z13 },
    { "VFENEZH", mi_VFENE, { { 3, 1 }, { 2 } }, UNI_SINCE_Z13 },
    { "VFENEZHS", mi_VFENE, { { 3, 1 }, { 3 } }, UNI_SINCE_Z13 },
    { "VFIDB", mi_VFI, { { 2, 3 } }, UNI_SINCE_Z13 },
    { "VFISB", mi_VFI, { { 2, 2 } }, UNI_SINCE_Z14 },
    { "VFKEDB", mi_VFCE, { { 3, 3 }, { 4 }, { 0 } }, UNI_SINCE_Z14 },
    { "VFKEDBS", mi_VFCE, { { 3, 3 }, { 4 }, { 1 } }, UNI_SINCE_Z14 },
    { "VFKESB", mi_VFCE, { { 3, 2 }, { 4 }, { 0 } }, UNI_SINCE_Z14 },
    { "VFKESBS", mi_VFCE, { { 3, 2 }, { 4 }, { 1 } }, UNI_SINCE_Z14 },
    { "VFKHDB", mi_VFCH, { { 3, 3 }, { 4 }, { 0 } }, UNI_SINCE_Z14 },
    { "VFKHDBS", mi_VFCH, { { 3, 3 }, { 4 }, { 1 } }, UNI_SINCE_Z14 },
    { "VFKHEDB", mi_VFCHE, { { 3, 3 }, { 4 }, { 0 } }, UNI_SINCE_Z14 },
    { "VFKHEDBS", mi_VFCHE, { { 3, 3 }, { 4 }, { 1 } }, UNI_SINCE_Z14 },
    { "VFKHESB", mi_VFCHE, { { 3, 2 }, { 4 }, { 0 } }, UNI_SINCE_Z14 },
    { "VFKHESBS", mi_VFCHE, { { 3, 2 }, { 4 }, { 1 } }, UNI_SINCE_Z14 },
    { "VFKHSB", mi_VFCH, { { 3, 2 }, { 4 }, { 0 } }, UNI_SINCE_Z14 },
    { "VFKHSBS", mi_VFCH, { { 3, 2 }, { 4 }, { 1 } }, UNI_SINCE_Z14 },
    { "VFLCDB", mi_VFPSO, { { 2, 3 }, { 0 }, { 0 } }, UNI_SINCE_Z13 },
    { "VFLCSB", mi_VFPSO, { { 2, 2 }, { 0 }, { 0 } }, UNI_SINCE_Z14 },
    { "VFLLS", mi_VFLL, { { 2, 2 }, { 0 } }, UNI_SINCE_Z14 },
    { "VFLNDB", mi_VFPSO, { { 2, 3 }, { 0 }, { 1 } }, UNI_SINCE_Z13 },
    { "VFLNSB", mi_VFPSO, { { 2, 2 }, { 0 }, { 1 } }, UNI_SINCE_Z14 },
    { "VFLPDB", mi_VFPSO, { { 2, 3 }, { 0 }, { 2 } }, UNI_SINCE_Z13 },
    { "VFLPSB", mi_VFPSO, { { 2, 2 }, { 0 }, { 2 } }, UNI_SINCE_Z14 },
    { "VFLRD", mi_VFLR, { { 2, 3 } }, UNI_SINCE_Z14 },
    { "VFMADB", mi_VFMA, { { 4, 0 }, { 3 } }, UNI_SINCE_Z13 },
    { "VFMASB", mi_VFMA, { { 4, 0 }, { 2 } }, UNI_SINCE_Z14 },
    { "VFMAXDB", mi_VFMAX, { { 3, 3 }, { 0 } }, UNI_SINCE_Z14 },
    { "VFMAXSB", mi_VFMAX, { { 3, 2 }, { 0 } }, UNI_SINCE_Z14 },
    { "VFMDB", mi_VFM, { { 3, 3 }, { 0 } }, UNI_SINCE_Z13 },
    { "VFMINDB", mi_VFMIN, { { 3, 3 }, { 0 } }, UNI_SINCE_Z14 },
    { "VFMINSB", mi_VFMIN, { { 3, 2 }, { 0 } }, UNI_SINCE_Z14 },
    { "VFMSB", mi_VFM, { { 3, 2 }, { 0 } }, UNI_SINCE_Z14 },
    { "VFMSDB", mi_VFMS, { { 4, 0 }, { 3 } }, UNI_SINCE_Z13 },
    { "VFMSSB", mi_VFMS, { { 4, 0 }, { 2 } }, UNI_SINCE_Z14 },
    { "VFNMADB", mi_VFNMA, { { 4, 0 }, { 3 } }, UNI_SINCE_Z14 },
    { "VFNMASB", mi_VFNMA, { { 4, 0 }, { 2 } }, UNI_SINCE_Z14 },
    { "VFNMSDB", mi_VFNMS, { { 4, 0 }, { 3 } }, UNI_SINCE_Z14 },
    { "VFNMSSB", mi_VFNMS, { { 4, 0 }, { 2 } }, UNI_SINCE_Z14 },
    { "VFPSODB", mi_VFPSO, { { 2, 3 }, { 0 } }, UNI_SINCE_Z13 },
    { "VFPSOSB", mi_VFPSO, { { 2, 2 }, { 0 } }, UNI_SINCE_Z14 },
    { "VFSDB", mi_VFS, { { 2, 3 }, { 0 } }, UNI_SINCE_Z13 },
    { "VFSQDB", mi_VFSQ, { { 2, 3 }, { 0 } }, UNI_SINCE_Z13 },
    { "VFSQSB", mi_VFSQ, { { 2, 2 }, { 0 } }, UNI_SINCE_Z14 },
    { "VFSSB", mi_VFS, { { 2, 2 }, { 0 } }, UNI_SINCE_Z14 },
    { "VFTCIDB", mi_VFTCI, { { 3, 3 }, { 0 } }, UNI_SINCE_Z13 },
    { "VFTCISB", mi_VFTCI, { { 3, 2 }, { 0 } }, UNI_SINCE_Z14 },
    { "VGFMAB", mi_VGFMA, { { 4, 0 } }, UNI_SINCE_Z13 },
    { "VGFMAF", mi_VGFMA, { { 4, 2 } }, UNI_SINCE_Z13 },
    { "VGFMAG", mi_VGFMA, { { 4, 3 } }, UNI_SINCE_Z13 },
    { "VGFMAH", mi_VGFMA, { { 4, 1 } }, UNI_SINCE_Z13 },
    { "VGFMB", mi_VGFM, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VGFMF", mi_VGFM, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VGFMG", mi_VGFM, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VGFMH", mi_VGFM, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VGMB", mi_VGM, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VGMF", mi_VGM, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VGMG", mi_VGM, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VGMH", mi_VGM, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VISTRB", mi_VISTR, { { 2, 0 } }, UNI_SINCE_Z13 },
    { "VISTRBS", mi_VISTR, { { 2, 0 }, { 1 } }, UNI_SINCE_Z13 },
    { "VISTRF", mi_VISTR, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "VISTRFS", mi_VISTR, { { 2, 2 }, { 1 } }, UNI_SINCE_Z13 },
    { "VISTRH", mi_VISTR, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "VISTRHS", mi_VISTR, { { 2, 1 }, { 1 } }, UNI_SINCE_Z13 },
    { "VLBRF", mi_VLBR, { { 2, 2 } }, UNI_SINCE_Z15 },
    { "VLBRG", mi_VLBR, { { 2, 3 } }, UNI_SINCE_Z15 },
    { "VLBRH", mi_VLBR, { { 2, 1 } }, UNI_SINCE_Z15 },
    { "VLBRQ", mi_VLBR, { { 2, 4 } }, UNI_SINCE_Z15 },
    { "VLBRREPF", mi_VLBRREP, { { 2, 2 } }, UNI_SINCE_Z15 },
    { "VLBRREPG", mi_VLBRREP, { { 2, 3 } }, UNI_SINCE_Z15 },
    { "VLBRREPH", mi_VLBRREP, { { 2, 1 } }, UNI_SINCE_Z15 },
    { "VLCB", mi_VLC, { { 2, 0 } }, UNI_SINCE_Z13 },
    { "VLCF", mi_VLC, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "VLCG", mi_VLC, { { 2, 3 } }, UNI_SINCE_Z13 },
    { "VLCH", mi_VLC, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "VLDE", mi_VFLL, {}, UNI_SINCE_Z13 },
    { "VLDEB", mi_VFLL, { { 2, 2 }, { 0 } }, UNI_SINCE_Z13 },
    { "VLED", mi_VFLR, {}, UNI_SINCE_Z13 },
    { "VLEDB", mi_VFLR, { { 2, 3 } }, UNI_SINCE_Z13 },
    { "VLERF", mi_VLER, { { 2, 2 } }, UNI_SINCE_Z15 },
    { "VLERG", mi_VLER, { { 2, 3 } }, UNI_SINCE_Z15 },
    { "VLERH", mi_VLER, { { 2, 1 } }, UNI_SINCE_Z15 },
    { "VLGVB", mi_VLGV, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VLGVF", mi_VLGV, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VLGVG", mi_VLGV, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VLGVH", mi_VLGV, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VLLEBRZE", mi_VLLEBRZ, { { 2, 6 } }, UNI_SINCE_Z15 },
    { "VLLEBRZF", mi_VLLEBRZ, { { 2, 2 } }, UNI_SINCE_Z15 },
    { "VLLEBRZG", mi_VLLEBRZ, { { 2, 3 } }, UNI_SINCE_Z15 },
    { "VLLEBRZH", mi_VLLEBRZ, { { 2, 1 } }, UNI_SINCE_Z15 },
    { "VLLEZB", mi_VLLEZ, { { 2, 0 } }, UNI_SINCE_Z13 },
    { "VLLEZF", mi_VLLEZ, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "VLLEZG", mi_VLLEZ, { { 2, 3 } }, UNI_SINCE_Z13 },
    { "VLLEZH", mi_VLLEZ, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "VLLEZLF", mi_VLLEZ, { { 2, 6 } }, UNI_SINCE_Z14 },
    { "VLPB", mi_VLP, { { 2, 0 } }, UNI_SINCE_Z13 },
    { "VLPF", mi_VLP, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "VLPG", mi_VLP, { { 2, 3 } }, UNI_SINCE_Z13 },
    { "VLPH", mi_VLP, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "VLREPB", mi_VLREP, { { 2, 0 } }, UNI_SINCE_Z13 },
    { "VLREPF", mi_VLREP, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "VLREPG", mi_VLREP, { { 2, 3 } }, UNI_SINCE_Z13 },
    { "VLREPH", mi_VLREP, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "VLVGB", mi_VLVG, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VLVGF", mi_VLVG, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VLVGG", mi_VLVG, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VLVGH", mi_VLVG, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VMAEB", mi_VMAE, { { 4, 0 } }, UNI_SINCE_Z13 },
    { "VMAEF", mi_VMAE, { { 4, 2 } }, UNI_SINCE_Z13 },
    { "VMAEH", mi_VMAE, { { 4, 1 } }, UNI_SINCE_Z13 },
    { "VMAHB", mi_VMAH, { { 4, 0 } }, UNI_SINCE_Z13 },
    { "VMAHF", mi_VMAH, { { 4, 2 } }, UNI_SINCE_Z13 },
    { "VMAHH", mi_VMAH, { { 4, 1 } }, UNI_SINCE_Z13 },
    { "VMALB", mi_VMAL, { { 4, 0 } }, UNI_SINCE_Z13 },
    { "VMALEB", mi_VMALE, { { 4, 0 } }, UNI_SINCE_Z13 },
    { "VMALEF", mi_VMALE, { { 4, 2 } }, UNI_SINCE_Z13 },
    { "VMALEH", mi_VMALE, { { 4, 1 } }, UNI_SINCE_Z13 },
    { "VMALF", mi_VMAL, { { 4, 2 } }, UNI_SINCE_Z13 },
    { "VMALHB", mi_VMALH, { { 4, 0 } }, UNI_SINCE_Z13 },
    { "VMALHF", mi_VMALH, { { 4, 2 } }, UNI_SINCE_Z13 },
    { "VMALHH", mi_VMALH, { { 4, 1 } }, UNI_SINCE_Z13 },
    { "VMALHW", mi_VMAL, { { 4, 1 } }, UNI_SINCE_Z13 },
    { "VMALOB", mi_VMALO, { { 4, 0 } }, UNI_SINCE_Z13 },
    { "VMALOF", mi_VMALO, { { 4, 2 } }, UNI_SINCE_Z13 },
    { "VMALOH", mi_VMALO, { { 4, 1 } }, UNI_SINCE_Z13 },
    { "VMAOB", mi_VMAO, { { 4, 0 } }, UNI_SINCE_Z13 },
    { "VMAOF", mi_VMAO, { { 4, 2 } }, UNI_SINCE_Z13 },
    { "VMAOH", mi_VMAO, { { 4, 1 } }, UNI_SINCE_Z13 },
    { "VMEB", mi_VME, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VMEF", mi_VME, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VMEH", mi_VME, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VMHB", mi_VMH, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VMHF", mi_VMH, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VMHH", mi_VMH, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VMLB", mi_VML, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VMLEB", mi_VMLE, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VMLEF", mi_VMLE, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VMLEH", mi_VMLE, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VMLF", mi_VML, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VMLHB", mi_VMLH, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VMLHF", mi_VMLH, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VMLHH", mi_VMLH, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VMLHW", mi_VML, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VMLOB", mi_VMLO, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VMLOF", mi_VMLO, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VMLOH", mi_VMLO, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VMNB", mi_VMN, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VMNF", mi_VMN, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VMNG", mi_VMN, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VMNH", mi_VMN, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VMNLB", mi_VMNL, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VMNLF", mi_VMNL, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VMNLG", mi_VMNL, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VMNLH", mi_VMNL, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VMOB", mi_VMO, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VMOF", mi_VMO, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VMOH", mi_VMO, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VMRHB", mi_VMRH, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VMRHF", mi_VMRH, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VMRHG", mi_VMRH, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VMRHH", mi_VMRH, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VMRLB", mi_VMRL, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VMRLF", mi_VMRL, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VMRLG", mi_VMRL, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VMRLH", mi_VMRL, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VMSLG", mi_VMSL, { { 4, 3 } }, UNI_SINCE_Z14 },
    { "VMXB", mi_VMX, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VMXF", mi_VMX, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VMXG", mi_VMX, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VMXH", mi_VMX, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VMXLB", mi_VMXL, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VMXLF", mi_VMXL, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VMXLG", mi_VMXL, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VMXLH", mi_VMXL, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VNOT", mi_VNO, { mnemonic_transformation(2, mnemonic_transformation_kind::copy, 1) }, UNI_SINCE_Z13 },
    { "VONE", mi_VGBM, { { 1, 65535 } }, UNI_SINCE_Z13 },
    { "VPKF", mi_VPK, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VPKG", mi_VPK, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VPKH", mi_VPK, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VPKLSF", mi_VPKLS, { { 3, 2 }, { 0 } }, UNI_SINCE_Z13 },
    { "VPKLSFS", mi_VPKLS, { { 3, 2 }, { 1 } }, UNI_SINCE_Z13 },
    { "VPKLSG", mi_VPKLS, { { 3, 3 }, { 0 } }, UNI_SINCE_Z13 },
    { "VPKLSGS", mi_VPKLS, { { 3, 3 }, { 1 } }, UNI_SINCE_Z13 },
    { "VPKLSH", mi_VPKLS, { { 3, 1 }, { 0 } }, UNI_SINCE_Z13 },
    { "VPKLSHS", mi_VPKLS, { { 3, 1 }, { 1 } }, UNI_SINCE_Z13 },
    { "VPKSF", mi_VPKS, { { 3, 2 }, { 0 } }, UNI_SINCE_Z13 },
    { "VPKSFS", mi_VPKS, { { 3, 2 }, { 1 } }, UNI_SINCE_Z13 },
    { "VPKSG", mi_VPKS, { { 3, 3 }, { 0 } }, UNI_SINCE_Z13 },
    { "VPKSGS", mi_VPKS, { { 3, 3 }, { 1 } }, UNI_SINCE_Z13 },
    { "VPKSH", mi_VPKS, { { 3, 1 }, { 0 } }, UNI_SINCE_Z13 },
    { "VPKSHS", mi_VPKS, { { 3, 1 }, { 1 } }, UNI_SINCE_Z13 },
    { "VPOPCTB", mi_VPOPCT, { { 2, 0 } }, UNI_SINCE_Z14 },
    { "VPOPCTF", mi_VPOPCT, { { 2, 2 } }, UNI_SINCE_Z14 },
    { "VPOPCTG", mi_VPOPCT, { { 2, 3 } }, UNI_SINCE_Z14 },
    { "VPOPCTH", mi_VPOPCT, { { 2, 1 } }, UNI_SINCE_Z14 },
    { "VREPB", mi_VREP, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VREPF", mi_VREP, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VREPG", mi_VREP, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VREPH", mi_VREP, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VREPIB", mi_VREPI, { { 2, 0 } }, UNI_SINCE_Z13 },
    { "VREPIF", mi_VREPI, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "VREPIG", mi_VREPI, { { 2, 3 } }, UNI_SINCE_Z13 },
    { "VREPIH", mi_VREPI, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "VSB", mi_VS, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VSBCBIQ", mi_VSBCBI, { { 4, 4 } }, UNI_SINCE_Z13 },
    { "VSBIQ", mi_VSBI, { { 4, 4 } }, UNI_SINCE_Z13 },
    { "VSCBIB", mi_VSCBI, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VSCBIF", mi_VSCBI, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VSCBIG", mi_VSCBI, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VSCBIH", mi_VSCBI, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VSCBIQ", mi_VSCBI, { { 3, 4 } }, UNI_SINCE_Z13 },
    { "VSCHDP", mi_VSCHP, { { 3, 3 } }, UNI_SINCE_Z16 },
    { "VSCHSP", mi_VSCHP, { { 3, 2 } }, UNI_SINCE_Z16 },
    { "VSCHXP", mi_VSCHP, { { 3, 4 } }, UNI_SINCE_Z16 },
    { "VSEGB", mi_VSEG, { { 2, 0 } }, UNI_SINCE_Z13 },
    { "VSEGF", mi_VSEG, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "VSEGH", mi_VSEG, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "VSF", mi_VS, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VSG", mi_VS, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VSH", mi_VS, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VSQ", mi_VS, { { 3, 4 } }, UNI_ESA_XA_370_SINCE_Z13 },
    { "VSTBRF", mi_VSTBR, { { 2, 2 } }, UNI_SINCE_Z15 },
    { "VSTBRG", mi_VSTBR, { { 2, 3 } }, UNI_SINCE_Z15 },
    { "VSTBRH", mi_VSTBR, { { 2, 1 } }, UNI_SINCE_Z15 },
    { "VSTBRQ", mi_VSTBR, { { 2, 4 } }, UNI_SINCE_Z15 },
    { "VSTERF", mi_VSTER, { { 2, 2 } }, UNI_SINCE_Z15 },
    { "VSTERG", mi_VSTER, { { 2, 3 } }, UNI_SINCE_Z15 },
    { "VSTERH", mi_VSTER, { { 2, 1 } }, UNI_SINCE_Z15 },
    { "VSTRCB", mi_VSTRC, { { 4, 0 } }, UNI_SINCE_Z13 },
    { "VSTRCBS",
        mi_VSTRC,
        { { 4, 0 }, mnemonic_transformation(0, 1, mnemonic_transformation_kind::or_with, 4, false) },
        UNI_SINCE_Z13 },
    { "VSTRCF", mi_VSTRC, { { 4, 2 } }, UNI_SINCE_Z13 },
    { "VSTRCFS",
        mi_VSTRC,
        { { 4, 2 }, mnemonic_transformation(0, 1, mnemonic_transformation_kind::or_with, 4, false) },
        UNI_SINCE_Z13 },
    { "VSTRCH", mi_VSTRC, { { 4, 1 } }, UNI_SINCE_Z13 },
    { "VSTRCHS",
        mi_VSTRC,
        { { 4, 1 }, mnemonic_transformation(0, 1, mnemonic_transformation_kind::or_with, 4, false) },
        UNI_SINCE_Z13 },
    { "VSTRCZB",
        mi_VSTRC,
        { { 4, 0 }, mnemonic_transformation(0, 2, mnemonic_transformation_kind::or_with, 4, false) },
        UNI_SINCE_Z13 },
    { "VSTRCZBS",
        mi_VSTRC,
        { { 4, 0 }, mnemonic_transformation(0, 3, mnemonic_transformation_kind::or_with, 4, false) },
        UNI_SINCE_Z13 },
    { "VSTRCZF",
        mi_VSTRC,
        { { 4, 2 }, mnemonic_transformation(0, 2, mnemonic_transformation_kind::or_with, 4, false) },
        UNI_SINCE_Z13 },
    { "VSTRCZFS",
        mi_VSTRC,
        { { 4, 2 }, mnemonic_transformation(0, 3, mnemonic_transformation_kind::or_with, 4, false) },
        UNI_SINCE_Z13 },
    { "VSTRCZH",
        mi_VSTRC,
        { { 4, 1 }, mnemonic_transformation(0, 2, mnemonic_transformation_kind::or_with, 4, false) },
        UNI_SINCE_Z13 },
    { "VSTRCZHS",
        mi_VSTRC,
        { { 4, 1 }, mnemonic_transformation(0, 3, mnemonic_transformation_kind::or_with, 4, false) },
        UNI_SINCE_Z13 },
    { "VSTRSB", mi_VSTRS, { { 4, 0 } }, UNI_SINCE_Z15 },
    { "VSTRSF", mi_VSTRS, { { 4, 2 } }, UNI_SINCE_Z15 },
    { "VSTRSH", mi_VSTRS, { { 4, 1 } }, UNI_SINCE_Z15 },
    { "VSTRSZB", mi_VSTRS, { { 4, 0 }, { 2 } }, UNI_SINCE_Z15 },
    { "VSTRSZF", mi_VSTRS, { { 4, 2 }, { 2 } }, UNI_SINCE_Z15 },
    { "VSTRSZH", mi_VSTRS, { { 4, 1 }, { 2 } }, UNI_SINCE_Z15 },
    { "VSUMB", mi_VSUM, { { 3, 0 } }, UNI_SINCE_Z13 },
    { "VSUMGF", mi_VSUMG, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VSUMGH", mi_VSUMG, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VSUMH", mi_VSUM, { { 3, 1 } }, UNI_SINCE_Z13 },
    { "VSUMQF", mi_VSUMQ, { { 3, 2 } }, UNI_SINCE_Z13 },
    { "VSUMQG", mi_VSUMQ, { { 3, 3 } }, UNI_SINCE_Z13 },
    { "VUPHB", mi_VUPH, { { 2, 0 } }, UNI_SINCE_Z13 },
    { "VUPHF", mi_VUPH, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "VUPHH", mi_VUPH, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "VUPLB", mi_VUPL, { { 2, 0 } }, UNI_SINCE_Z13 },
    { "VUPLF", mi_VUPL, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "VUPLHB", mi_VUPLH, { { 2, 0 } }, UNI_SINCE_Z13 },
    { "VUPLHF", mi_VUPLH, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "VUPLHH", mi_VUPLH, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "VUPLHW", mi_VUPL, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "VUPLLB", mi_VUPLL, { { 2, 0 } }, UNI_SINCE_Z13 },
    { "VUPLLF", mi_VUPLL, { { 2, 2 } }, UNI_SINCE_Z13 },
    { "VUPLLH", mi_VUPLL, { { 2, 1 } }, UNI_SINCE_Z13 },
    { "VZERO", mi_VGBM, { { 1 } }, UNI_SINCE_Z13 },
    { "WCDGB",
        mi_VCFPS,
        { { 2, 3 }, mnemonic_transformation(0, 8, mnemonic_transformation_kind::or_with, 2, false) },
        UNI_SINCE_Z13 },
    { "WCDLGB",
        mi_VCFPL,
        { { 2, 3 }, mnemonic_transformation(0, 8, mnemonic_transformation_kind::or_with, 2, false) },
        UNI_SINCE_Z13 },
    { "WCEFB",
        mi_VCFPS,
        { { 2, 2 }, mnemonic_transformation(0, 8, mnemonic_transformation_kind::or_with, 2, false) },
        UNI_SINCE_Z15 },
    { "WCELFB",
        mi_VCFPL,
        { { 2, 2 }, mnemonic_transformation(0, 8, mnemonic_transformation_kind::or_with, 2, false) },
        UNI_SINCE_Z15 },
    { "WCFEB",
        mi_VCSFP,
        { { 2, 2 }, mnemonic_transformation(0, 8, mnemonic_transformation_kind::or_with, 2, false) },
        UNI_SINCE_Z15 },
    { "WCGDB",
        mi_VCSFP,
        { { 2, 3 }, mnemonic_transformation(0, 8, mnemonic_transformation_kind::or_with, 2, false) },
        UNI_SINCE_Z13 },
    { "WCLFEB",
        mi_VCLFP,
        { { 2, 2 }, mnemonic_transformation(0, 8, mnemonic_transformation_kind::or_with, 2, false) },
        UNI_SINCE_Z15 },
    { "WCLGDB",
        mi_VCLFP,
        { { 2, 3 }, mnemonic_transformation(0, 8, mnemonic_transformation_kind::or_with, 2, false) },
        UNI_SINCE_Z13 },
    { "WFADB", mi_VFA, { { 3, 3 }, { 8 } }, UNI_SINCE_Z13 },
    { "WFASB", mi_VFA, { { 3, 2 }, { 8 } }, UNI_SINCE_Z14 },
    { "WFAXB", mi_VFA, { { 3, 4 }, { 8 } }, UNI_SINCE_Z14 },
    { "WFCDB", mi_WFC, { { 2, 3 }, { 0 } }, UNI_SINCE_Z13 },
    { "WFCEDB", mi_VFCE, { { 3, 3 }, { 8 }, { 0 } }, UNI_SINCE_Z13 },
    { "WFCEDBS", mi_VFCE, { { 3, 3 }, { 8 }, { 1 } }, UNI_SINCE_Z13 },
    { "WFCESB", mi_VFCE, { { 3, 2 }, { 8 }, { 0 } }, UNI_SINCE_Z14 },
    { "WFCESBS", mi_VFCE, { { 3, 2 }, { 8 }, { 1 } }, UNI_SINCE_Z14 },
    { "WFCEXB", mi_VFCE, { { 3, 4 }, { 8 }, { 0 } }, UNI_SINCE_Z14 },
    { "WFCEXBS", mi_VFCE, { { 3, 4 }, { 8 }, { 1 } }, UNI_SINCE_Z14 },
    { "WFCHDB", mi_VFCH, { { 3, 3 }, { 8 }, { 0 } }, UNI_SINCE_Z13 },
    { "WFCHDBS", mi_VFCH, { { 3, 3 }, { 8 }, { 1 } }, UNI_SINCE_Z13 },
    { "WFCHEDB", mi_VFCHE, { { 3, 3 }, { 8 }, { 0 } }, UNI_SINCE_Z13 },
    { "WFCHEDBS", mi_VFCHE, { { 3, 3 }, { 8 }, { 1 } }, UNI_SINCE_Z13 },
    { "WFCHESB", mi_VFCHE, { { 3, 2 }, { 8 }, { 0 } }, UNI_SINCE_Z14 },
    { "WFCHESBS", mi_VFCHE, { { 3, 2 }, { 8 }, { 1 } }, UNI_SINCE_Z14 },
    { "WFCHEXB", mi_VFCHE, { { 3, 4 }, { 8 }, { 0 } }, UNI_SINCE_Z14 },
    { "WFCHEXBS", mi_VFCHE, { { 3, 4 }, { 8 }, { 1 } }, UNI_SINCE_Z14 },
    { "WFCHSB", mi_VFCH, { { 3, 2 }, { 8 }, { 0 } }, UNI_SINCE_Z14 },
    { "WFCHSBS", mi_VFCH, { { 3, 2 }, { 8 }, { 1 } }, UNI_SINCE_Z14 },
    { "WFCHXB", mi_VFCH, { { 3, 4 }, { 8 }, { 0 } }, UNI_SINCE_Z14 },
    { "WFCHXBS", mi_VFCH, { { 3, 4 }, { 8 }, { 1 } }, UNI_SINCE_Z14 },
    { "WFCSB", mi_WFC, { { 2, 2 }, { 0 } }, UNI_SINCE_Z14 },
    { "WFCXB", mi_WFC, { { 2, 4 }, { 0 } }, UNI_SINCE_Z14 },
    { "WFDDB", mi_VFD, { { 3, 3 }, { 8 } }, UNI_SINCE_Z13 },
    { "WFDSB", mi_VFD, { { 3, 2 }, { 8 } }, UNI_SINCE_Z14 },
    { "WFDXB", mi_VFD, { { 3, 4 }, { 8 } }, UNI_SINCE_Z14 },
    { "WFIDB",
        mi_VFI,
        { { 2, 3 }, mnemonic_transformation(0, 8, mnemonic_transformation_kind::or_with, 2, false) },
        UNI_SINCE_Z13 },
    { "WFISB",
        mi_VFI,
        { { 2, 2 }, mnemonic_transformation(0, 8, mnemonic_transformation_kind::or_with, 2, false) },
        UNI_SINCE_Z14 },
    { "WFIXB",
        mi_VFI,
        { { 2, 4 }, mnemonic_transformation(0, 8, mnemonic_transformation_kind::or_with, 2, false) },
        UNI_SINCE_Z14 },
    { "WFKDB", mi_WFK, { { 2, 3 }, { 0 } }, UNI_SINCE_Z13 },
    { "WFKEDB", mi_VFCE, { { 3, 3 }, { 12 }, { 0 } }, UNI_SINCE_Z14 },
    { "WFKEDBS", mi_VFCE, { { 3, 3 }, { 12 }, { 1 } }, UNI_SINCE_Z14 },
    { "WFKESB", mi_VFCE, { { 3, 2 }, { 12 }, { 0 } }, UNI_SINCE_Z14 },
    { "WFKESBS", mi_VFCE, { { 3, 2 }, { 12 }, { 1 } }, UNI_SINCE_Z14 },
    { "WFKEXB", mi_VFCE, { { 3, 4 }, { 12 }, { 0 } }, UNI_SINCE_Z14 },
    { "WFKEXBS", mi_VFCE, { { 3, 4 }, { 12 }, { 1 } }, UNI_SINCE_Z14 },
    { "WFKHDB", mi_VFCH, { { 3, 3 }, { 12 }, { 0 } }, UNI_SINCE_Z14 },
    { "WFKHDBS", mi_VFCH, { { 3, 3 }, { 12 }, { 1 } }, UNI_SINCE_Z14 },
    { "WFKHEDB", mi_VFCHE, { { 3, 3 }, { 12 }, { 0 } }, UNI_SINCE_Z14 },
    { "WFKHEDBS", mi_VFCHE, { { 3, 3 }, { 12 }, { 1 } }, UNI_SINCE_Z14 },
    { "WFKHESB", mi_VFCHE, { { 3, 2 }, { 12 }, { 0 } }, UNI_SINCE_Z14 },
    { "WFKHESBS", mi_VFCHE, { { 3, 2 }, { 12 }, { 1 } }, UNI_SINCE_Z14 },
    { "WFKHEXB", mi_VFCHE, { { 3, 4 }, { 12 }, { 0 } }, UNI_SINCE_Z14 },
    { "WFKHEXBS", mi_VFCHE, { { 3, 4 }, { 12 }, { 1 } }, UNI_SINCE_Z14 },
    { "WFKHSB", mi_VFCH, { { 3, 2 }, { 12 }, { 0 } }, UNI_SINCE_Z14 },
    { "WFKHSBS", mi_VFCH, { { 3, 2 }, { 12 }, { 1 } }, UNI_SINCE_Z14 },
    { "WFKHXB", mi_VFCH, { { 3, 4 }, { 12 }, { 0 } }, UNI_SINCE_Z14 },
    { "WFKHXBS", mi_VFCH, { { 3, 4 }, { 12 }, { 1 } }, UNI_SINCE_Z14 },
    { "WFKSB", mi_WFK, { { 2, 2 }, { 0 } }, UNI_SINCE_Z14 },
    { "WFKXB", mi_WFK, { { 2, 4 }, { 0 } }, UNI_SINCE_Z14 },
    { "WFLCDB", mi_VFPSO, { { 2, 3 }, { 8 }, { 0 } }, UNI_SINCE_Z13 },
    { "WFLCSB", mi_VFPSO, { { 2, 2 }, { 8 }, { 0 } }, UNI_SINCE_Z14 },
    { "WFLCXB", mi_VFPSO, { { 2, 4 }, { 8 }, { 0 } }, UNI_SINCE_Z14 },
    { "WFLLD", mi_VFLL, { { 2, 3 }, { 8 } }, UNI_SINCE_Z14 },
    { "WFLLS", mi_VFLL, { { 2, 2 }, { 8 } }, UNI_SINCE_Z14 },
    { "WFLNDB", mi_VFPSO, { { 2, 3 }, { 8 }, { 1 } }, UNI_SINCE_Z13 },
    { "WFLNSB", mi_VFPSO, { { 2, 2 }, { 8 }, { 1 } }, UNI_SINCE_Z14 },
    { "WFLNXB", mi_VFPSO, { { 2, 4 }, { 8 }, { 1 } }, UNI_SINCE_Z14 },
    { "WFLPDB", mi_VFPSO, { { 2, 3 }, { 8 }, { 2 } }, UNI_SINCE_Z13 },
    { "WFLPSB", mi_VFPSO, { { 2, 2 }, { 8 }, { 2 } }, UNI_SINCE_Z14 },
    { "WFLPXB", mi_VFPSO, { { 2, 4 }, { 8 }, { 2 } }, UNI_SINCE_Z14 },
    { "WFLRD",
        mi_VFLR,
        { { 2, 3 }, mnemonic_transformation(0, 8, mnemonic_transformation_kind::or_with, 2, false) },
        UNI_SINCE_Z14 },
    { "WFLRX",
        mi_VFLR,
        { { 2, 4 }, mnemonic_transformation(0, 8, mnemonic_transformation_kind::or_with, 2, false) },
        UNI_SINCE_Z14 },
    { "WFMADB", mi_VFMA, { { 4, 8 }, { 3 } }, UNI_SINCE_Z13 },
    { "WFMASB", mi_VFMA, { { 4, 8 }, { 2 } }, UNI_SINCE_Z14 },
    { "WFMAXB", mi_VFMA, { { 4, 8 }, { 4 } }, UNI_SINCE_Z14 },
    { "WFMAXDB", mi_VFMAX, { { 3, 3 }, { 8 } }, UNI_SINCE_Z14 },
    { "WFMAXSB", mi_VFMAX, { { 3, 2 }, { 8 } }, UNI_SINCE_Z14 },
    { "WFMAXXB", mi_VFMAX, { { 3, 4 }, { 8 } }, UNI_SINCE_Z14 },
    { "WFMDB", mi_VFM, { { 3, 3 }, { 8 } }, UNI_SINCE_Z13 },
    { "WFMINDB", mi_VFMIN, { { 3, 3 }, { 8 } }, UNI_SINCE_Z14 },
    { "WFMINSB", mi_VFMIN, { { 3, 2 }, { 8 } }, UNI_SINCE_Z14 },
    { "WFMINXB", mi_VFMIN, { { 3, 4 }, { 8 } }, UNI_SINCE_Z14 },
    { "WFMSB", mi_VFM, { { 3, 2 }, { 8 } }, UNI_SINCE_Z14 },
    { "WFMSDB", mi_VFMS, { { 4, 8 }, { 3 } }, UNI_SINCE_Z13 },
    { "WFMSSB", mi_VFMS, { { 4, 8 }, { 2 } }, UNI_SINCE_Z14 },
    { "WFMSXB", mi_VFMS, { { 4, 8 }, { 4 } }, UNI_SINCE_Z14 },
    { "WFMXB", mi_VFM, { { 3, 4 }, { 8 } }, UNI_SINCE_Z14 },
    { "WFNMADB", mi_VFNMA, { { 4, 8 }, { 3 } }, UNI_SINCE_Z14 },
    { "WFNMASB", mi_VFNMA, { { 4, 8 }, { 2 } }, UNI_SINCE_Z14 },
    { "WFNMAXB", mi_VFNMA, { { 4, 8 }, { 4 } }, UNI_SINCE_Z14 },
    { "WFNMSDB", mi_VFNMS, { { 4, 8 }, { 3 } }, UNI_SINCE_Z14 },
    { "WFNMSSB", mi_VFNMS, { { 4, 8 }, { 2 } }, UNI_SINCE_Z14 },
    { "WFNMSXB", mi_VFNMS, { { 4, 8 }, { 4 } }, UNI_SINCE_Z14 },
    { "WFPSODB", mi_VFPSO, { { 2, 3 }, { 8 } }, UNI_SINCE_Z13 },
    { "WFPSOSB", mi_VFPSO, { { 2, 2 }, { 8 } }, UNI_SINCE_Z14 },
    { "WFPSOXB", mi_VFPSO, { { 2, 4 }, { 8 } }, UNI_SINCE_Z14 },
    { "WFSDB", mi_VFS, { { 2, 3 }, { 8 } }, UNI_SINCE_Z13 },
    { "WFSQDB", mi_VFSQ, { { 2, 3 }, { 8 } }, UNI_SINCE_Z13 },
    { "WFSQSB", mi_VFSQ, { { 2, 2 }, { 8 } }, UNI_SINCE_Z14 },
    { "WFSQXB", mi_VFSQ, { { 2, 4 }, { 8 } }, UNI_SINCE_Z14 },
    { "WFSSB", mi_VFS, { { 2, 2 }, { 8 } }, UNI_SINCE_Z14 },
    { "WFSXB", mi_VFS, { { 2, 4 }, { 8 } }, UNI_SINCE_Z14 },
    { "WFTCIDB", mi_VFTCI, { { 3, 3 }, { 8 } }, UNI_SINCE_Z13 },
    { "WFTCISB", mi_VFTCI, { { 3, 2 }, { 8 } }, UNI_SINCE_Z14 },
    { "WFTCIXB", mi_VFTCI, { { 3, 4 }, { 8 } }, UNI_SINCE_Z14 },
    { "WLDEB", mi_VFLL, { { 2, 2 }, { 8 } }, UNI_SINCE_Z13 },
    { "WLEDB",
        mi_VFLR,
        { { 2, 3 }, mnemonic_transformation(0, 8, mnemonic_transformation_kind::or_with, 2, false) },
        UNI_SINCE_Z13 },
    { "XHHR", mi_RXSBG, { { 2, 0 }, { 31 } }, UNI_SINCE_Z11 },
    { "XHLR", mi_RXSBG, { { 2, 0 }, { 31 }, { 32 } }, UNI_SINCE_Z11 },
    { "XLHR", mi_RXSBG, { { 2, 32 }, { 63 }, { 32 } }, UNI_SINCE_Z11 },
};

static_assert(std::ranges::is_sorted(mnemonic_codes, {}, &mnemonic_code::name));

const mnemonic_code* instruction::find_mnemonic_codes(std::string_view name) noexcept
{
    auto it = std::ranges::lower_bound(mnemonic_codes, name, {}, &mnemonic_code::name);
    if (it == std::ranges::end(mnemonic_codes) || it->name() != name)
        return nullptr;
    return std::to_address(it);
}

const mnemonic_code& instruction::get_mnemonic_codes(std::string_view name) noexcept
{
    auto result = find_mnemonic_codes(name);
    assert(result);
    return *result;
}

std::span<const mnemonic_code> instruction::all_mnemonic_codes() noexcept { return mnemonic_codes; }

namespace {
constexpr std::size_t combined_machine_instruction_name_limit = 8;
constexpr const auto combined_machine_instruction_table = []() {
    constexpr const auto limit = combined_machine_instruction_name_limit;
    std::array<std::array<char, limit>, std::size(machine_instructions) + std::size(mnemonic_codes)> names {};
    std::array<short, std::size(machine_instructions) + std::size(mnemonic_codes)> offsets {};

    const auto m1 = (std::size_t)-1;
    std::size_t out_i = 0;
    std::size_t mach_i = 0;
    std::size_t mnemo_i = 0;

    for (; mach_i < std::size(machine_instructions) && mnemo_i < std::size(mnemonic_codes); ++out_i)
    {
        const auto ma = machine_instructions[mach_i].name();
        const auto mn = mnemonic_codes[mnemo_i].name();
        const auto [name, idx] = ma <= mn ? std::pair(ma, mach_i++) : std::pair(mn, m1 - mnemo_i++);
        assert(name.size() <= limit);
        std::ranges::copy(name, names[out_i].begin());
        offsets[out_i] = (short)idx;
    }

    for (; mach_i < std::size(machine_instructions); ++mach_i, ++out_i)
    {
        const auto& m = machine_instructions[mach_i];
        const auto name = m.name();
        assert(name.size() <= limit);
        std::ranges::copy(name, names[out_i].begin());
        offsets[out_i] = (short)mach_i;
    }

    for (; mnemo_i < std::size(mnemonic_codes); ++mnemo_i, ++out_i)
    {
        const auto& mn = mnemonic_codes[mnemo_i];
        const auto name = mn.name();
        assert(name.size() <= limit);
        std::ranges::copy(name, names[out_i].begin());
        offsets[out_i] = (short)(m1 - mnemo_i);
    }

    return std::pair(names, offsets);
}();
} // namespace

std::pair<const machine_instruction*, const mnemonic_code*> instruction::find_machine_instruction_or_mnemonic(
    std::string_view name) noexcept
{
    if (name.size() > combined_machine_instruction_name_limit)
        return {};

    const auto& [mach_instr_names, mach_instr_offsets] = combined_machine_instruction_table;

    std::array<char, combined_machine_instruction_name_limit> padded_name {};
    std::ranges::copy(name, padded_name.begin());

    auto it = std::ranges::lower_bound(mach_instr_names, padded_name);
    if (it == std::end(mach_instr_names) || *it != padded_name)
        return {};

    const auto idx = mach_instr_offsets[it - std::begin(mach_instr_names)];

    if (idx >= 0)
        return { &machine_instructions[idx], nullptr };

    const auto& mn = mnemonic_codes[-idx - 1];

    return { mn.instruction(), &mn };
}

namespace {
constexpr instruction_set_size compute_instruction_set_size(instruction_set_version v)
{
    instruction_set_size result = {
        0,
        0,
        std::size(ca_instructions),
        std::size(assembler_instructions),
    };
    for (const auto& i : machine_instructions)
        if (instruction_available(i.instr_set_affiliation(), v))
            ++result.machine;
    for (const auto& i : mnemonic_codes)
        if (instruction_available(i.instr_set_affiliation(), v))
            ++result.mnemonic;

    return result;
}

constexpr const instruction_set_size instruction_set_sizes[] = {
    {},
    compute_instruction_set_size(instruction_set_version::ZOP),
    compute_instruction_set_size(instruction_set_version::YOP),
    compute_instruction_set_size(instruction_set_version::Z9),
    compute_instruction_set_size(instruction_set_version::Z10),
    compute_instruction_set_size(instruction_set_version::Z11),
    compute_instruction_set_size(instruction_set_version::Z12),
    compute_instruction_set_size(instruction_set_version::Z13),
    compute_instruction_set_size(instruction_set_version::Z14),
    compute_instruction_set_size(instruction_set_version::Z15),
    compute_instruction_set_size(instruction_set_version::Z16),
    compute_instruction_set_size(instruction_set_version::ESA),
    compute_instruction_set_size(instruction_set_version::XA),
    compute_instruction_set_size(instruction_set_version::_370),
    compute_instruction_set_size(instruction_set_version::DOS),
    compute_instruction_set_size(instruction_set_version::UNI),
};

} // namespace

const instruction_set_size& hlasm_plugin::parser_library::context::get_instruction_sizes(
    instruction_set_version v) noexcept
{
    auto idx = static_cast<int>(v);
    assert(0 < idx && idx < std::size(instruction_set_sizes));
    return instruction_set_sizes[idx];
}
