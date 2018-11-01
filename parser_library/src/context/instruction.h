#pragma once
#include <array>
#include <string>
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

struct machine_instruction {
	std::string name;
	size_t operands_count;
	mach_format format;
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

//static class holding string names of instructions with theirs additional info
class instruction
{
public:
	static const std::vector<std::string> ca_instructions;

	static const std::vector<assembler_instruction> assembler_instructions;

	static const std::vector<machine_instruction> machine_instructions;
};


}
}
}
