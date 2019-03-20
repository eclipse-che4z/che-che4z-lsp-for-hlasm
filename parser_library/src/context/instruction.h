#ifndef HLASMPLUGIN_PARSERLIBRARY_CONTEXT_INSTRUCTION_H
#define HLASMPLUGIN_PARSERLIBRARY_CONTEXT_INSTRUCTION_H

#include <array>
#include <string>
#include <map>

#include <vector>
#include "id_storage.h"

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
	SS_f, SSE, SSF, VRI_a, VRI_b, VRI_c, VRI_d, VRI_e, VRR_a, VRR_b,
	VRR_c, VRR_d, VRR_e, VRR_f, VRS_a, VRS_b, VRS_c, VRV, VRX
};

enum class operand_format {
	SIZE_4b, SIZE_8b, SIZE_12b, SIZE_16b, SIZE_24b, SIZE_32b,
	DXB_4b, DXB_8b, DB
};

struct machine_instruction
{
	//machine_instruction(std::string name, std::vector<operand_format> operands, mach_format format) : name(name), format(format), operands(operands), has_optional_operand(false) {}
	//machine_instruction(std::string name, std::vector<operand_format> operands, mach_format format, bool has_optional_operand) : name(name), format(format), operands(operands), has_optional_operand(has_optional_operand) {}
	std::string name;
	mach_format format;
	std::vector<operand_format> operands;
	size_t size_for_alloc;
	bool has_optional_operand;
};

/*
min_operands - minimal number of operands, non-negative integer, always defined
max_operands - if not defined (can be infinite), value is -1, otherwise a non-negative integer
*/
struct assembler_instruction {
	std::string name;
	int min_operands;
	int max_operands;

	assembler_instruction() = delete;
	assembler_instruction(std::string name, int min_operands, int max_operands) : name(std::move(name)), min_operands(min_operands), max_operands(max_operands) {}
};

struct extended_mnemonic_codes
{
	std::string extended_branch;
	int operand;
	std::string machine_instr;
};

//static class holding string names of instructions with theirs additional info
class instruction
{
public:
	static const std::vector<std::string> ca_instructions;

	static const std::vector<std::string> macro_processing_instructions;

	static const std::vector<assembler_instruction> assembler_instructions;

	static const std::vector<machine_instruction> machine_instructions;
	
	static const std::vector<extended_mnemonic_codes> mnemonic_codes;
};

}
}
}

#endif
