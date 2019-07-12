#ifndef HLASMPLUGIN_PARSERLIBRARY_CONTEXT_INSTRUCTION_H
#define HLASMPLUGIN_PARSERLIBRARY_CONTEXT_INSTRUCTION_H

#include <array>
#include <string>
#include <map>

#include <vector>
#include "id_storage.h"
#include "../checking/instr_operand.h"
#include "../diagnostic.h"

namespace hlasm_plugin {
namespace parser_library {
	namespace context {

		enum class instruction_type
		{
			MACH, MAC, ASM, CA, DAT
		};

		//all mach_format types for operands of machine instructions:
		enum class mach_format {
			E, I, IE, MII, RI_a, RI_b, RI_c, RIE_a, RIE_b, RIE_c,
			RIE_d, RIE_e, RIE_f, RIE_g, RIL_a, RIL_b, RIL_c, RIS, RR, RRD,
			RRE, RRF_a, RRF_b, RRF_c, RRF_d, RRF_e, RRS, RS_a, RS_b, RSI,
			RSL_a, RSL_b, RSY_a, RSY_b, RX_a, RX_b, RXE, RXF, RXY_a, RXY_b,
			S, SI, SIL, SIY, SMI, SS_a, SS_b, SS_c, SS_d, SS_e,
			SS_f, SSE, SSF, VRI_a, VRI_b, VRI_c, VRI_d, VRI_e, VRI_f, VRR_a, VRR_b,
			VRR_c, VRR_d, VRR_e, VRR_f, VRS_a, VRS_b, VRS_c, VRV, VRX,
			VRI_g, VRI_h, VRI_i, VRR_g, VRR_h, VRR_i, VRS_d, VSI
		};

		const checking::parameter empty = { false, 0, hlasm_plugin::parser_library::checking::machine_operand_type::NONE };
		const checking::parameter reg = { false, 4, hlasm_plugin::parser_library::checking::machine_operand_type::REG };
		const checking::parameter dis_reg = { false, 4, hlasm_plugin::parser_library::checking::machine_operand_type::DIS_REG };
		const checking::parameter dis_reg_r = { false, 4, hlasm_plugin::parser_library::checking::machine_operand_type::REG };
		const checking::parameter mask = { false, 4, hlasm_plugin::parser_library::checking::machine_operand_type::MASK };
		const checking::parameter dis_12u = { false, 12, hlasm_plugin::parser_library::checking::machine_operand_type::DISPLC };
		const checking::parameter dis_20s = { true, 12, hlasm_plugin::parser_library::checking::machine_operand_type::DISPLC };
		const checking::parameter base = { false, 4, hlasm_plugin::parser_library::checking::machine_operand_type::BASE };
		const checking::parameter length_8 = { false, 8, hlasm_plugin::parser_library::checking::machine_operand_type::LENGTH };
		const checking::parameter length_4 = { false, 4, hlasm_plugin::parser_library::checking::machine_operand_type::LENGTH };
		const checking::parameter imm_4u = { false, 4, hlasm_plugin::parser_library::checking::machine_operand_type::IMM };
		const checking::parameter imm_8s = { true, 8, hlasm_plugin::parser_library::checking::machine_operand_type::IMM };
		const checking::parameter imm_8u = { false, 8, hlasm_plugin::parser_library::checking::machine_operand_type::IMM };
		const checking::parameter imm_12s = { true, 12, hlasm_plugin::parser_library::checking::machine_operand_type::IMM };
		const checking::parameter imm_16s = { true, 16, hlasm_plugin::parser_library::checking::machine_operand_type::IMM };
		const checking::parameter imm_16u = { false, 16, hlasm_plugin::parser_library::checking::machine_operand_type::IMM };
		const checking::parameter imm_24s = { true, 24, hlasm_plugin::parser_library::checking::machine_operand_type::IMM };
		const checking::parameter imm_32s = { true, 32, hlasm_plugin::parser_library::checking::machine_operand_type::IMM };
		const checking::parameter vec_reg = { false, 4, hlasm_plugin::parser_library::checking::machine_operand_type::VEC_REG };
		const checking::parameter reg_imm_12s = { true, 12, hlasm_plugin::parser_library::checking::machine_operand_type::REG_IMM };
		const checking::parameter reg_imm_16s = { true, 16, hlasm_plugin::parser_library::checking::machine_operand_type::REG_IMM };
		const checking::parameter reg_imm_24s = { true, 24, hlasm_plugin::parser_library::checking::machine_operand_type::REG_IMM };
		const checking::parameter reg_imm_32s = { true, 32, hlasm_plugin::parser_library::checking::machine_operand_type::REG_IMM };

		/*
		Rules for displacement operands:
		With DB formats
			- must be in format D(B), otherwise throw an error
			- parser returns this in (displacement, 0, base, true) format
		With DXB Formats
			- can be either D(X,B) or D(,B) - in this case, the X is replaced with 0
			- parser returns this in (displacement, x, base, false) format
		*/
		const hlasm_plugin::parser_library::checking::machine_operand_format db_12_4_U = hlasm_plugin::parser_library::checking::machine_operand_format(dis_12u, empty, base);
		const hlasm_plugin::parser_library::checking::machine_operand_format db_20_4_S = hlasm_plugin::parser_library::checking::machine_operand_format(dis_20s, empty, base);
		const hlasm_plugin::parser_library::checking::machine_operand_format drb_12_4x4_U = hlasm_plugin::parser_library::checking::machine_operand_format(dis_12u, dis_reg_r, base);
		const hlasm_plugin::parser_library::checking::machine_operand_format dxb_12_4x4_U = hlasm_plugin::parser_library::checking::machine_operand_format(dis_12u, dis_reg, base);
		const hlasm_plugin::parser_library::checking::machine_operand_format dxb_20_4x4_S = hlasm_plugin::parser_library::checking::machine_operand_format(dis_20s, dis_reg, base);
		const hlasm_plugin::parser_library::checking::machine_operand_format dvb_12_4x4_U = hlasm_plugin::parser_library::checking::machine_operand_format(dis_20s, vec_reg, base);
		const hlasm_plugin::parser_library::checking::machine_operand_format reg_4_U = hlasm_plugin::parser_library::checking::machine_operand_format(reg, empty, empty);
		const hlasm_plugin::parser_library::checking::machine_operand_format mask_4_U = hlasm_plugin::parser_library::checking::machine_operand_format(mask, empty, empty);
		const hlasm_plugin::parser_library::checking::machine_operand_format imm_4_U = hlasm_plugin::parser_library::checking::machine_operand_format(imm_4u, empty, empty);
		const hlasm_plugin::parser_library::checking::machine_operand_format imm_8_S = hlasm_plugin::parser_library::checking::machine_operand_format(imm_8s, empty, empty);
		const hlasm_plugin::parser_library::checking::machine_operand_format imm_8_U = hlasm_plugin::parser_library::checking::machine_operand_format(imm_8u, empty, empty);
		const hlasm_plugin::parser_library::checking::machine_operand_format imm_16_U = hlasm_plugin::parser_library::checking::machine_operand_format(imm_16u, empty, empty);
		const hlasm_plugin::parser_library::checking::machine_operand_format imm_12_S = hlasm_plugin::parser_library::checking::machine_operand_format(imm_12s, empty, empty);
		const hlasm_plugin::parser_library::checking::machine_operand_format imm_16_S = hlasm_plugin::parser_library::checking::machine_operand_format(imm_16s, empty, empty);
		const hlasm_plugin::parser_library::checking::machine_operand_format imm_32_S = hlasm_plugin::parser_library::checking::machine_operand_format(imm_32s, empty, empty);
		const hlasm_plugin::parser_library::checking::machine_operand_format vec_reg_4_U = hlasm_plugin::parser_library::checking::machine_operand_format(vec_reg, empty, empty);
		const hlasm_plugin::parser_library::checking::machine_operand_format db_12_8x4L_U = hlasm_plugin::parser_library::checking::machine_operand_format(dis_12u, length_8, base);
		const hlasm_plugin::parser_library::checking::machine_operand_format db_12_4x4L_U = hlasm_plugin::parser_library::checking::machine_operand_format(dis_12u, length_4, base);
		const hlasm_plugin::parser_library::checking::machine_operand_format reg_imm_12_S = hlasm_plugin::parser_library::checking::machine_operand_format(reg_imm_12s, empty, empty);
		const hlasm_plugin::parser_library::checking::machine_operand_format reg_imm_16_S = hlasm_plugin::parser_library::checking::machine_operand_format(reg_imm_16s, empty, empty);
		const hlasm_plugin::parser_library::checking::machine_operand_format reg_imm_24_S = hlasm_plugin::parser_library::checking::machine_operand_format(reg_imm_24s, empty, empty);
		const hlasm_plugin::parser_library::checking::machine_operand_format reg_imm_32_S = hlasm_plugin::parser_library::checking::machine_operand_format(reg_imm_32s, empty, empty);

struct diag_range
{
public:
	diagnostic_op diag;
	semantics::symbol_range range;

	diag_range() : diag(diagnostic_op::error_NOERR()), range(semantics::symbol_range(0, 0, 0, 0)) {};
	diag_range(diagnostic_op diag, semantics::symbol_range range) : diag(diag), range(range) {};

	diag_range(diag_range&& d) noexcept :diag(d.diag), range(std::move(d.range)) {};
	diag_range(const diag_range&d) : diag(d.diag), range(std::move(d.range)) {};
};

class machine_instruction
{
public:
	std::string instr_name;
	mach_format format;
	std::vector<checking::machine_operand_format> operands; // what the vector of operands should look like
	size_t size_for_alloc;
	int no_optional;
	size_t page_no;

	machine_instruction(const std::string & name, mach_format format, std::vector<checking::machine_operand_format> operands, int no_optional, size_t size, size_t page_no)
		: instr_name(name), format(format), operands(operands), size_for_alloc(size), no_optional(no_optional), page_no(page_no) {};
	machine_instruction(const std::string & name, mach_format format, std::vector<checking::machine_operand_format> operands, size_t size, size_t page_no)
		:machine_instruction(name, format, operands, 0, size, page_no) {}

	virtual bool check(std::string & name_of_instruction, std::vector<hlasm_plugin::parser_library::checking::machine_operand_value*> operands); //input vector is the vector of the actual incoming values

	//std::vector<diag_range> & get_diagnostics();
	void clear_diagnostics();

	std::vector<diag_range> diagnostics;

	virtual ~machine_instruction() = default;
};

using machine_instruction_ptr = std::unique_ptr<machine_instruction>;

struct mnemonic_code
{
public:
	mnemonic_code(std::string instr, std::vector<std::pair<size_t, size_t>> replaced) : instruction(instr), replaced(replaced) {};

	std::string instruction;
	// first goes place, then value
	std::vector<std::pair<size_t, size_t>> replaced;
};

//static class holding string names of instructions with theirs additional info
class instruction
{
public:

	static std::map<const std::string, machine_instruction_ptr> get_machine_instructions();

	static std::map<const std::string, mnemonic_code> get_mnemonic_codes();

	static const std::vector<std::string> ca_instructions;

	/*
	min_operands - minimal number of operands, non-negative integer, always defined
	max_operands - if not defined (can be infinite), value is -1, otherwise a non-negative integer
	*/
	static const std::map<const std::string, const std::pair<int, int>> assembler_instructions;

	static const std::vector<std::string> macro_processing_instructions;

	static std::map<const std::string, machine_instruction_ptr> machine_instructions;

	static const std::map<const std::string, mnemonic_code> mnemonic_codes;

	static const std::map<mach_format, const std::string> mach_format_to_string;
};


}
}
}

#endif
