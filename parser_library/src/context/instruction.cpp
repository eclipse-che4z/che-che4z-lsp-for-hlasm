#include "instruction.h"

using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library::checking;
using namespace hlasm_plugin::parser_library;

const std::map<mach_format, const std::string> instruction::mach_format_to_string = {
	{mach_format::E,"E"},
	{mach_format::I,"I"},
	{mach_format::IE,"IE"},
	{mach_format::MII,"MII"},
	{mach_format::RI_a,"RI-a"},
	{mach_format::RI_b,"RI-b"},
	{mach_format::RI_c,"RI-c"},
	{mach_format::RIE_a,"RIE-a"},
	{mach_format::RIE_b,"RIE-b"},
	{mach_format::RIE_c,"RIE-c"},
	{mach_format::RIE_d,"RIE-d"},
	{mach_format::RIE_e,"RIE-e"},
	{mach_format::RIE_f,"RIE-f"},
	{mach_format::RIE_g,"RIE-g"},
	{mach_format::RIL_a,"RIL-a"},
	{mach_format::RIL_b,"RIL-b"},
	{mach_format::RIL_c,"RIL-c"},
	{mach_format::RIS,"RIS"},
	{mach_format::RR,"RR"},
	{mach_format::RRD,"RRD"},
	{mach_format::RRE,"RRE"},
	{mach_format::RRF_a,"RRF-a"},
	{mach_format::RRF_b,"RRF-b"},
	{mach_format::RRF_c,"RRF-c"},
	{mach_format::RRF_d,"RRF-d"},
	{mach_format::RRF_e,"RRF-e"},
	{mach_format::RRS,"RRS"},
	{mach_format::RS_a,"RS-a"},
	{mach_format::RS_b,"RS-b"},
	{mach_format::RSI,"RSI"},
	{mach_format::RSL_a,"RSL-a"},
	{mach_format::RSL_b,"RSL-b"},
	{mach_format::RSY_a,"RSY-a"},
	{mach_format::RSY_b,"RSY-b"},
	{mach_format::RX_a,"RX-a"},
	{mach_format::RX_b,"RX-b"},
	{mach_format::RXE,"RXE"},
	{mach_format::RXF,"RXF"},
	{mach_format::RXY_a,"RXY-a"},
	{mach_format::RXY_b,"RXY-b"},
	{mach_format::S,"S"},
	{mach_format::SI,"SI"},
	{mach_format::SIL,"SIL"},
	{mach_format::SIY,"SIY"},
	{mach_format::SMI,"SMI"},
	{mach_format::SS_a,"SS-a"},
	{mach_format::SS_b,"SS-b"},
	{mach_format::SS_c,"SS-c"},
	{mach_format::SS_d,"SS-d"},
	{mach_format::SS_e,"SS-e"},
	{mach_format::SS_f,"SS-f"},
	{mach_format::SSE,"SSE"},
	{mach_format::SSF,"SSF"},
	{mach_format::VRI_a,"VRI-a"},
	{mach_format::VRI_b,"VRI-b"},
	{mach_format::VRI_c,"VRI-c"},
	{mach_format::VRI_d,"VRI-d"},
	{mach_format::VRI_e,"VRI-e"},
	{mach_format::VRI_f,"VRI-f"},
	{mach_format::VRI_g,"VRI-g"},
	{mach_format::VRI_h,"VRI-h"},
	{mach_format::VRI_i,"VRI-i"},
	{mach_format::VRR_a,"VRR-a"},
	{mach_format::VRR_b,"VRR-b"},
	{mach_format::VRR_c,"VRR-c"},
	{mach_format::VRR_d,"VRR-d"},
	{mach_format::VRR_e,"VRR-e"},
	{mach_format::VRR_f,"VRR-f"},
	{mach_format::VRR_g,"VRR-g"},
	{mach_format::VRR_h,"VRR-h"},
	{mach_format::VRR_i,"VRR-i"},
	{mach_format::VRS_a,"VRS-a"},
	{mach_format::VRS_b,"VRS-b"},
	{mach_format::VRS_c,"VRS-c"},
	{mach_format::VRS_d,"VRS-d"},
	{mach_format::VSI,"VSI"},
	{mach_format::VRV,"VRV"},
	{mach_format::VRX,"VRX"}
};

const std::vector<ca_instruction> instruction::ca_instructions = {
	{"AIF",false},
	{"AGO",false},
	{"ACTR",false},
	{"SETA",false},
	{"SETB",false},
	{"SETC",false},
	{"ANOP",true},
	{"LCLA",false},
	{"LCLB",false},
	{"LCLC",false},
	{"GBLA",false},
	{"GBLB",false},
	{"GBLC",false},
	{"MACRO",true},
	{"MEND",true},
	{"MEXIT",true},
	{"AEJECT",true},
	{"AREAD",false},
	{"ASPACE",false}
};

const std::map<const std::string, assembler_instruction> instruction::assembler_instructions = { {
	{ "*PROCESS", {1,-1, true} }, // TO DO
	{ "ACONTROL", {1,-1, false} },
	{ "ADATA", {5,5, false} },
	{ "AINSERT", {2,2, false} },
	{ "ALIAS", {1,1, false} },
	{ "AMODE", {1,1, false} },
	{ "CATTR", {1,-1, false} },
	{ "CCW", {4,4, true} },
	{ "CCW0",  {4,4, true} },
	{ "CCW1", {4,4, true} },
	{ "CEJECT", {0,1, true} },
	{ "CNOP", {2,2, true} },
	{ "COM", {0,0, false} },
	{ "COPY", {1,1, false} },
	{ "CSECT", {0,0, false} },
	{ "CXD", {0,0, false} },
	{ "DC", {1,-1, true} },
	{ "DROP", {0,-1, true} },
	{ "DS", {1,-1, true} },
	{ "DSECT", {0,0, false} },
	{ "DXD", {1,-1, true} },
	{ "EJECT", {0,0, false} },
	{ "END", {0,2, true} },
	{ "ENTRY", {1,-1, true} },
	{ "EQU", {1,5, true} },
	{ "EXITCTL", {2,5, false} },
	{ "EXTRN", {1,-1, false} },
	{ "ICTL", {1,3, false} },
	{ "ISEQ", {0,2, false} },
	{ "LOCTR", {0,0, false} },
	{ "LTORG", {0,0, false} },
	{ "MNOTE", {1,2, false} },
	{ "OPSYN", {0,1, true} },
	{ "ORG", {0,3, true} },
	{ "POP", {1,4, true} },
	{ "PRINT", {1,-1, false } },
	{ "PUNCH", {1,1, false} },
	{ "PUSH", {1,4, false} },
	{ "REPRO", {0,0, false} },
	{ "RMODE", {1,1, false} },
	{ "RSECT", {0,0, false } },
	{ "SPACE", {0,1, true} },
	{ "START", {0,1, true} },
	{ "TITLE", {1,1, false} },
	{ "USING", {2,-1, true} },
	{ "WXTRN", {1,-1, false} },
	{ "XATTR", {1,-1, false} }
}};

bool hlasm_plugin::parser_library::context::machine_instruction::check(const std::string& name_of_instruction, const std::vector<const checking::machine_operand*> to_check)
{
	// check size of operands
	int diff = operands.size() - to_check.size();
	if (diff > no_optional || diff < 0)
	{
		auto diag = diagnostic_op::error_optional_number_of_operands(name_of_instruction, no_optional, operands.size());
		diag_range curr_diag = diag_range(diag, range());
		diagnostics.push_back(curr_diag);
		return false;
	}
	bool error = false;
	for (size_t i = 0; i < to_check.size(); i++)
	{
		if ((to_check[i]) != nullptr)
		{
			diag_range diagnostic;
			if (!(*to_check[i]).check(diagnostic.diag, operands[i], name_of_instruction))
			{
				diagnostic.diagnostic_range = (*to_check[i]).operand_range;
				diagnostics.push_back(std::move(diagnostic));
				error = true;
			}
		}
		else
		{
			hlasm_plugin::parser_library::checking::machine_operand temp;
			auto diag = temp.get_address_operand_expected(operands[i], name_of_instruction);
			diag_range curr_diag = diag_range(diag, range());
			diagnostics.push_back(curr_diag);
			error = true;
		}
	};
	return (!error); 
}

void hlasm_plugin::parser_library::context::machine_instruction::clear_diagnostics()
{
	diagnostics.clear();
}

class vnot_instruction : public machine_instruction
{
public:
	vnot_instruction(const std::string& name, mach_format format, std::vector<machine_operand_format> operands, size_t size, size_t page_no)
		:machine_instruction(name, format, operands, size, page_no, 0) {}

	virtual bool check(const std::string& name_of_instruction,const std::vector<const hlasm_plugin::parser_library::checking::machine_operand*> to_check) override
	{
		if (!machine_instruction::check(name_of_instruction, to_check))
			return false;
		if (to_check.size() == 3)
		{
			try
			{
				const one_operand& second = dynamic_cast<const one_operand&>(*to_check[1]);
				const one_operand& third = dynamic_cast<const one_operand&>(*to_check[2]);
				if (second.value == third.value)
					return true;
				else
				{
					diagnostics.push_back(diag_range(diagnostic_op::error_M200(name_of_instruction), range()));
					return false;
				}
			}
			catch (...)
			{
				assert(false);
			}
		}
		diagnostics.push_back(diag_range(diagnostic_op::error_M000(name_of_instruction, operands.size()), range()));
		return false;
	}
};

std::map<const std::string, machine_instruction_ptr> hlasm_plugin::parser_library::context::instruction::get_machine_instructions()
{
	std::map<const std::string, machine_instruction_ptr> result;
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AR", std::make_unique<machine_instruction>("AR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U}, 16, 510)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AGR", std::make_unique<machine_instruction>("AGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U}, 32, 510)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AGFR", std::make_unique<machine_instruction>("AGFR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U}, 32, 510)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ARK", std::make_unique<machine_instruction>("ARK", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U}, 32, 510)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AGRK", std::make_unique<machine_instruction>("AGRK", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U}, 32, 510)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("A", std::make_unique<machine_instruction>("A", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U}, 32, 510)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AY", std::make_unique<machine_instruction>("AY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S}, 48, 511)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AG", std::make_unique<machine_instruction>("AG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S}, 48, 511)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AGF", std::make_unique<machine_instruction>("AGF", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S}, 48, 511)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AFI", std::make_unique<machine_instruction>("AFI", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 511)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AGFI", std::make_unique<machine_instruction>("AGFI", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 511)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AHIK", std::make_unique<machine_instruction>("AHIK", mach_format::RIE_d, std::vector<machine_operand_format>{reg_4_U, reg_4_U, imm_16_S  }, 48, 511)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AGHIK", std::make_unique<machine_instruction>("AGHIK", mach_format::RIE_d, std::vector<machine_operand_format>{reg_4_U, reg_4_U, imm_16_S  }, 48, 511)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ASI", std::make_unique<machine_instruction>("ASI", mach_format::SIY, std::vector<machine_operand_format>{ db_20_4_S, imm_8_S }, 48, 511)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AGSI", std::make_unique<machine_instruction>("AGSI", mach_format::SIY, std::vector<machine_operand_format>{db_20_4_S, imm_8_S }, 48, 511)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AH", std::make_unique<machine_instruction>("AH", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 512)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AHY", std::make_unique<machine_instruction>("AHY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 512)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AGH", std::make_unique<machine_instruction>("AGH", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 512)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AHI", std::make_unique<machine_instruction>("AHI", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 512)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AGHI", std::make_unique<machine_instruction>("AGHI", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 513)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AHHHR", std::make_unique<machine_instruction>("AHHHR", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U, }, 32, 513)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AHHLR", std::make_unique<machine_instruction>("AHHLR", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 513)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AIH", std::make_unique<machine_instruction>("AIH", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 513)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALR", std::make_unique<machine_instruction>("ALR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 514)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALGR", std::make_unique<machine_instruction>("ALGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 514)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALGFR", std::make_unique<machine_instruction>("ALGFR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 514)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALRK", std::make_unique<machine_instruction>("ALRK", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 514)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALGRK", std::make_unique<machine_instruction>("ALGRK", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 514)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AL", std::make_unique<machine_instruction>("AL", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 514)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALY", std::make_unique<machine_instruction>("ALY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 514)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALG", std::make_unique<machine_instruction>("ALG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 514)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALGF", std::make_unique<machine_instruction>("ALGF", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 514)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALFI", std::make_unique<machine_instruction>("ALFI", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 514)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALGFI", std::make_unique<machine_instruction>("ALGFI", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 514)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALHHHR", std::make_unique<machine_instruction>("ALHHHR", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U}, 32, 515)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALHHLR", std::make_unique<machine_instruction>("ALHHLR", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U}, 32, 515)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALCR", std::make_unique<machine_instruction>("ALCR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 515)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALCGR", std::make_unique<machine_instruction>("ALCGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 515)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALC", std::make_unique<machine_instruction>("ALC", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 515)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALCG", std::make_unique<machine_instruction>("ALCG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 515)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALSI", std::make_unique<machine_instruction>("ALSI", mach_format::SIY, std::vector<machine_operand_format>{db_20_4_S, imm_8_S }, 48, 516)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALGSI", std::make_unique<machine_instruction>("ALGSI", mach_format::SIY, std::vector<machine_operand_format>{db_20_4_S, imm_8_S }, 48, 516)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALHSIK", std::make_unique<machine_instruction>("ALHSIK", mach_format::RIE_d, std::vector<machine_operand_format>{reg_4_U, reg_4_U, imm_16_S  }, 48, 516)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALGHSIK", std::make_unique<machine_instruction>("ALGHSIK", mach_format::RIE_d, std::vector<machine_operand_format>{reg_4_U, reg_4_U, imm_16_S  }, 48, 516)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALSIH", std::make_unique<machine_instruction>("ALSIH", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 517)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ALSIHN", std::make_unique<machine_instruction>("ALSIHN", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 517)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("NR", std::make_unique<machine_instruction>("NR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 517)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("NGR", std::make_unique<machine_instruction>("NGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 517)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("NRK", std::make_unique<machine_instruction>("NRK", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 517)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("NGRK", std::make_unique<machine_instruction>("NGRK", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 517)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("N", std::make_unique<machine_instruction>("N", mach_format::RX_a, std::vector<machine_operand_format>{ reg_4_U, dxb_12_4x4_U }, 32, 517)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("NY", std::make_unique<machine_instruction>("NY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 517)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("NG", std::make_unique<machine_instruction>("NG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 517)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("NI", std::make_unique<machine_instruction>("NI", mach_format::SI, std::vector<machine_operand_format>{db_12_4_U, imm_8_S }, 32, 517)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("NIY", std::make_unique<machine_instruction>("NIY", mach_format::SIY, std::vector<machine_operand_format>{db_20_4_S, imm_8_S }, 48, 518)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("NC", std::make_unique<machine_instruction>("NC", mach_format::SS_a, std::vector<machine_operand_format>{ db_12_8x4L_U, db_12_4_U }, 48, 518)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("NIHF", std::make_unique<machine_instruction>("NIHF", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 518)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("NIHH", std::make_unique<machine_instruction>("NIHH", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 518)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("NIHL", std::make_unique<machine_instruction>("NIHL", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 518)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("NILF", std::make_unique<machine_instruction>("NILF", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 519)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("NILH", std::make_unique<machine_instruction>("NILH", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 519)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("NILL", std::make_unique<machine_instruction>("NILL", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 519)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BALR", std::make_unique<machine_instruction>("BALR", mach_format::RR, std::vector<machine_operand_format>{ reg_4_U, reg_4_U }, 16, 519)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BAL", std::make_unique<machine_instruction>("BAL", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 519)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BASR", std::make_unique<machine_instruction>("BASR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 520)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BAS", std::make_unique<machine_instruction>("BAS", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 520)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BASSM", std::make_unique<machine_instruction>("BASSM", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 520)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BSM", std::make_unique<machine_instruction>("BSM", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 522)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BIC", std::make_unique<machine_instruction>("BIC", mach_format::RXY_b, std::vector<machine_operand_format>{mask_4_U, dxb_20_4x4_S }, 48, 523)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BCR", std::make_unique<machine_instruction>("BCR", mach_format::RR, std::vector<machine_operand_format>{mask_4_U, reg_4_U }, 16, 524)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BC", std::make_unique<machine_instruction>("BC", mach_format::RX_b, std::vector<machine_operand_format>{mask_4_U, dxb_12_4x4_U }, 32, 524)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BCTR", std::make_unique<machine_instruction>("BCTR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 525)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BCTGR", std::make_unique<machine_instruction>("BCTGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 525)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BCT", std::make_unique<machine_instruction>("BCT", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 525)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BCTG", std::make_unique<machine_instruction>("BCTG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 525)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BXH", std::make_unique<machine_instruction>("BXH", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_12_4_U }, 32, 526)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BXHG", std::make_unique<machine_instruction>("BXHG", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S, }, 48, 526)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BXLE", std::make_unique<machine_instruction>("BXLE", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_12_4_U }, 32, 526)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BXLEG", std::make_unique<machine_instruction>("BXLEG", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S, }, 48, 526)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BPP", std::make_unique<machine_instruction>("BPP", mach_format::SMI, std::vector<machine_operand_format>{mask_4_U, reg_imm_16_S, db_12_4_U }, 48, 527)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BPRP", std::make_unique<machine_instruction>("BPRP", mach_format::MII, std::vector<machine_operand_format>{mask_4_U, reg_imm_12_S, reg_imm_24_S }, 48, 527)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BRAS", std::make_unique<machine_instruction>("BRAS", mach_format::RI_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_16_S }, 32, 530)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BRASL", std::make_unique<machine_instruction>("BRASL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 48, 530)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BRC", std::make_unique<machine_instruction>("BRC", mach_format::RI_c, std::vector<machine_operand_format>{mask_4_U, reg_imm_16_S }, 32, 530)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BRCL", std::make_unique<machine_instruction>("BRCL", mach_format::RIL_c, std::vector<machine_operand_format>{mask_4_U, reg_imm_32_S }, 48, 530)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BRCT", std::make_unique<machine_instruction>("BRCT", mach_format::RI_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_16_S }, 32, 531)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BRCTG", std::make_unique<machine_instruction>("BRCTG", mach_format::RI_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_16_S }, 32, 531)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BRCTH", std::make_unique<machine_instruction>("BRCTH", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 48, 531)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BRXH", std::make_unique<machine_instruction>("BRXH", mach_format::RSI, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_imm_16_S }, 32, 532)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BRXHG", std::make_unique<machine_instruction>("BRXHG", mach_format::RIE_e, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_imm_16_S }, 48, 532)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BRXLE", std::make_unique<machine_instruction>("BRXLE", mach_format::RSI, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_imm_16_S }, 32, 532)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BRXLG", std::make_unique<machine_instruction>("BRXLG", mach_format::RIE_e, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_imm_16_S }, 48, 532)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CKSM", std::make_unique<machine_instruction>("CKSM", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 533)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("KM", std::make_unique<machine_instruction>("KM", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 537)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("KMC", std::make_unique<machine_instruction>("KMC", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 537)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("KMA", std::make_unique<machine_instruction>("KMA", mach_format::RRF_b, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 562)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("KMF", std::make_unique<machine_instruction>("KMF", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 576)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("KMCTR", std::make_unique<machine_instruction>("KMCTR", mach_format::RRF_b, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 591)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("KMO", std::make_unique<machine_instruction>("KMO", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 604)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CR", std::make_unique<machine_instruction>("CR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 618)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGR", std::make_unique<machine_instruction>("CGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 618)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGFR", std::make_unique<machine_instruction>("CGFR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 618)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("C", std::make_unique<machine_instruction>("C", mach_format::RX_a, std::vector<machine_operand_format>{ reg_4_U, dxb_12_4x4_U }, 32, 618)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CY", std::make_unique<machine_instruction>("CY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 618)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CG", std::make_unique<machine_instruction>("CG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 618)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGF", std::make_unique<machine_instruction>("CGF", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 618)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CFI", std::make_unique<machine_instruction>("CFI", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 618)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGFI", std::make_unique<machine_instruction>("CGFI", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 619)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CRL", std::make_unique<machine_instruction>("CRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 48, 619)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGRL", std::make_unique<machine_instruction>("CGRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 48, 619)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGFRL", std::make_unique<machine_instruction>("CGFRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 48, 619)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CRB", std::make_unique<machine_instruction>("CRB", mach_format::RRS, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U, db_12_4_U }, 48, 619)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGRB", std::make_unique<machine_instruction>("CGRB", mach_format::RRS, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U, db_12_4_U }, 48, 619)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CRJ", std::make_unique<machine_instruction>("CRJ", mach_format::RIE_b, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U, reg_imm_16_S }, 48, 619)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGRJ", std::make_unique<machine_instruction>("CGRJ", mach_format::RIE_b, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U, reg_imm_16_S }, 48, 620)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CIB", std::make_unique<machine_instruction>("CIB", mach_format::RIS, std::vector<machine_operand_format>{reg_4_U, imm_8_S, mask_4_U, db_12_4_U, }, 48, 620)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGIB", std::make_unique<machine_instruction>("CGIB", mach_format::RIS, std::vector<machine_operand_format>{reg_4_U, imm_8_S, mask_4_U, db_12_4_U, }, 48, 620)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CIJ", std::make_unique<machine_instruction>("CIJ", mach_format::RIE_c, std::vector<machine_operand_format>{reg_4_U, imm_8_S, mask_4_U, reg_imm_16_S }, 48, 620)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGIJ", std::make_unique<machine_instruction>("CGIJ", mach_format::RIE_c, std::vector<machine_operand_format>{reg_4_U, imm_8_S, mask_4_U, reg_imm_16_S }, 48, 620)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CFC", std::make_unique<machine_instruction>("CFC", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 621)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CS", std::make_unique<machine_instruction>("CS", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_12_4_U }, 32, 628)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CSY", std::make_unique<machine_instruction>("CSY", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 628)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CSG", std::make_unique<machine_instruction>("CSG", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 628)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDS", std::make_unique<machine_instruction>("CDS", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_12_4_U  }, 32, 628)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDSY", std::make_unique<machine_instruction>("CDSY", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 628)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDSG", std::make_unique<machine_instruction>("CDSG", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 628)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CSST", std::make_unique<machine_instruction>("CSST", mach_format::SSF, std::vector<machine_operand_format>{db_12_4_U, db_12_4_U, reg_4_U }, 48, 630)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CRT", std::make_unique<machine_instruction>("CRT", mach_format::RRF_c, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 32, 633)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGRT", std::make_unique<machine_instruction>("CGRT", mach_format::RRF_c, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 32, 633)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CIT", std::make_unique<machine_instruction>("CIT", mach_format::RIE_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S , mask_4_U }, 48, 633)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGIT", std::make_unique<machine_instruction>("CGIT", mach_format::RIE_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S , mask_4_U }, 48, 633)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CH", std::make_unique<machine_instruction>("CH", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 634)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CHY", std::make_unique<machine_instruction>("CHY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 634)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGH", std::make_unique<machine_instruction>("CGH", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 634)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CHI", std::make_unique<machine_instruction>("CHI", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 634)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGHI", std::make_unique<machine_instruction>("CGHI", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 634)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CHHSI", std::make_unique<machine_instruction>("CHHSI", mach_format::SIL, std::vector<machine_operand_format>{db_12_4_U, imm_16_S  }, 48, 634)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CHSI", std::make_unique<machine_instruction>("CHSI", mach_format::SIL, std::vector<machine_operand_format>{db_12_4_U, imm_16_S  }, 48, 634)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGHSI", std::make_unique<machine_instruction>("CGHSI", mach_format::SIL, std::vector<machine_operand_format>{db_12_4_U, imm_16_S  }, 48, 634)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CHRL", std::make_unique<machine_instruction>("CHRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 48, 634)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGHRL", std::make_unique<machine_instruction>("CGHRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 48, 634)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CHHR", std::make_unique<machine_instruction>("CHHR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 635)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CHLR", std::make_unique<machine_instruction>("CHLR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 635)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CHF", std::make_unique<machine_instruction>("CHF", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 635)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CIH", std::make_unique<machine_instruction>("CIH", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 635)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLR", std::make_unique<machine_instruction>("CLR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 636)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLGR", std::make_unique<machine_instruction>("CLGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 636)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLGFR", std::make_unique<machine_instruction>("CLGFR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 636)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CL", std::make_unique<machine_instruction>("CL", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 636)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLY", std::make_unique<machine_instruction>("CLY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 636)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLG", std::make_unique<machine_instruction>("CLG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 636)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLGF", std::make_unique<machine_instruction>("CLGF", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 636)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLC", std::make_unique<machine_instruction>("CLC", mach_format::SS_a, std::vector<machine_operand_format>{ db_12_8x4L_U, db_12_4_U, }, 48, 636)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLFI", std::make_unique<machine_instruction>("CLFI", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 636)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLGFI", std::make_unique<machine_instruction>("CLGFI", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 32, 636)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLI", std::make_unique<machine_instruction>("CLI", mach_format::SI, std::vector<machine_operand_format>{db_12_4_U, imm_8_S }, 48, 636)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLIY", std::make_unique<machine_instruction>("CLIY", mach_format::SIY, std::vector<machine_operand_format>{db_12_4_U, imm_8_S }, 48, 636)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLFHSI", std::make_unique<machine_instruction>("CLFHSI", mach_format::SIL, std::vector<machine_operand_format>{db_12_4_U, imm_16_S  }, 48, 636)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLGHSI", std::make_unique<machine_instruction>("CLGHSI", mach_format::SIL, std::vector<machine_operand_format>{db_12_4_U, imm_16_S  }, 48, 636)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLHHSI", std::make_unique<machine_instruction>("CLHHSI", mach_format::SIL, std::vector<machine_operand_format>{db_12_4_U, imm_16_S  }, 48, 636)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLRL", std::make_unique<machine_instruction>("CLRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 48, 637)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLGRL", std::make_unique<machine_instruction>("CLGRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 48, 637)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLGFRL", std::make_unique<machine_instruction>("CLGFRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 48, 637)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLHRL", std::make_unique<machine_instruction>("CLHRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 48, 637)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLGHRL", std::make_unique<machine_instruction>("CLGHRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 48, 637)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLRB", std::make_unique<machine_instruction>("CLRB", mach_format::RRS, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U, db_12_4_U }, 48, 638)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLGRB", std::make_unique<machine_instruction>("CLGRB", mach_format::RRS, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U, db_12_4_U }, 48, 638)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLRJ", std::make_unique<machine_instruction>("CLRJ", mach_format::RIE_b, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U, reg_imm_16_S }, 48, 638)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLGRJ", std::make_unique<machine_instruction>("CLGRJ", mach_format::RIE_b, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U, reg_imm_16_S }, 48, 638)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLIB", std::make_unique<machine_instruction>("CLIB", mach_format::RIS, std::vector<machine_operand_format>{reg_4_U, imm_8_S, mask_4_U, db_12_4_U }, 48, 638)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLGIB", std::make_unique<machine_instruction>("CLGIB", mach_format::RIS, std::vector<machine_operand_format>{reg_4_U, imm_8_S, mask_4_U, db_12_4_U }, 48, 638)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLIJ", std::make_unique<machine_instruction>("CLIJ", mach_format::RIE_c, std::vector<machine_operand_format>{reg_4_U, imm_8_S, mask_4_U, reg_imm_16_S }, 48, 638)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLGIJ", std::make_unique<machine_instruction>("CLGIJ", mach_format::RIE_c, std::vector<machine_operand_format>{reg_4_U, imm_8_S, mask_4_U, reg_imm_16_S }, 48, 638)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLRT", std::make_unique<machine_instruction>("CLRT", mach_format::RRF_c, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 32, 639)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLGRT", std::make_unique<machine_instruction>("CLGRT", mach_format::RRF_c, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 32, 639)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLT", std::make_unique<machine_instruction>("CLT", mach_format::RSY_b, std::vector<machine_operand_format>{reg_4_U, mask_4_U, dxb_20_4x4_S }, 48, 639)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLGT", std::make_unique<machine_instruction>("CLGT", mach_format::RSY_b, std::vector<machine_operand_format>{reg_4_U, mask_4_U, dxb_20_4x4_S }, 48, 639)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLFIT", std::make_unique<machine_instruction>("CLFIT", mach_format::RIE_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S , mask_4_U }, 48, 640)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLGIT", std::make_unique<machine_instruction>("CLGIT", mach_format::RIE_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S , mask_4_U }, 48, 640)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLM", std::make_unique<machine_instruction>("CLM", mach_format::RS_b, std::vector<machine_operand_format>{reg_4_U, mask_4_U, db_12_4_U }, 32, 641)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLMY", std::make_unique<machine_instruction>("CLMY", mach_format::RSY_b, std::vector<machine_operand_format>{reg_4_U, mask_4_U, db_20_4_S }, 48, 641)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLMH", std::make_unique<machine_instruction>("CLMH", mach_format::RSY_b, std::vector<machine_operand_format>{reg_4_U, mask_4_U, db_20_4_S  }, 48, 641)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLHHR", std::make_unique<machine_instruction>("CLHHR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 641)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLHLR", std::make_unique<machine_instruction>("CLHLR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 641)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLHF", std::make_unique<machine_instruction>("CLHF", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 641)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLCL", std::make_unique<machine_instruction>("CLCL", mach_format::RR, std::vector<machine_operand_format>{ reg_4_U, reg_4_U }, 16, 642)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLIH", std::make_unique<machine_instruction>("CLIH", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S}, 48, 642)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLCLE", std::make_unique<machine_instruction>("CLCLE", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_12_4_U }, 32, 644)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLCLU", std::make_unique<machine_instruction>("CLCLU", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 647)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLST", std::make_unique<machine_instruction>("CLST", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 650)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CUSE", std::make_unique<machine_instruction>("CUSE", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 651)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CMPSC", std::make_unique<machine_instruction>("CMPSC", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 654)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("KIMD", std::make_unique<machine_instruction>("KIMD", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 672)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("KLMD", std::make_unique<machine_instruction>("KLMD", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 685)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("KMAC", std::make_unique<machine_instruction>("KMAC", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 703)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CVB", std::make_unique<machine_instruction>("CVB", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 714)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CVBY", std::make_unique<machine_instruction>("CVBY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 714)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CVBG", std::make_unique<machine_instruction>("CVBG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 714)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CVD", std::make_unique<machine_instruction>("CVD", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 715)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CVDY", std::make_unique<machine_instruction>("CVDY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 715)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CVDG", std::make_unique<machine_instruction>("CVDG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 715)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CU24", std::make_unique<machine_instruction>("CU24", mach_format::RRF_c, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 1, 32, 715)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CUUTF", std::make_unique<machine_instruction>("CUUTF", mach_format::RRF_c, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 32, 718)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CU21", std::make_unique<machine_instruction>("CU21", mach_format::RRF_c, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 32, 718)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CU42", std::make_unique<machine_instruction>("CU42", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 722)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CU41", std::make_unique<machine_instruction>("CU41", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 725)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CUTFU", std::make_unique<machine_instruction>("CUTFU", mach_format::RRF_c, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 1, 32, 728)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CU12", std::make_unique<machine_instruction>("CU12", mach_format::RRF_c, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U  }, 1, 32, 728)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CU14", std::make_unique<machine_instruction>("CU14", mach_format::RRF_c, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 1, 32, 732)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CPYA", std::make_unique<machine_instruction>("CPYA", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 736)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DR", std::make_unique<machine_instruction>("DR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 736)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("D", std::make_unique<machine_instruction>("D", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U}, 32, 736)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DLR", std::make_unique<machine_instruction>("DLR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 737)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DLGR", std::make_unique<machine_instruction>("DLGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 737)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DL", std::make_unique<machine_instruction>("DL", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 737)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DLG", std::make_unique<machine_instruction>("DLG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 737)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DSGR", std::make_unique<machine_instruction>("DSGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 738)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DSGFR", std::make_unique<machine_instruction>("DSGFR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 738)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DSG", std::make_unique<machine_instruction>("DSG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 738)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DSGF", std::make_unique<machine_instruction>("DSGF", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 738)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("XR", std::make_unique<machine_instruction>("XR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 738)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("XGR", std::make_unique<machine_instruction>("XGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 738)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("XRK", std::make_unique<machine_instruction>("XRK", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 738)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("XGRK", std::make_unique<machine_instruction>("XGRK", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 738)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("X", std::make_unique<machine_instruction>("X", mach_format::RX_a, std::vector<machine_operand_format>{ reg_4_U, dxb_12_4x4_U}, 32, 738)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("XY", std::make_unique<machine_instruction>("XY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 738)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("XG", std::make_unique<machine_instruction>("XG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 738)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("XI", std::make_unique<machine_instruction>("XI", mach_format::SI, std::vector<machine_operand_format>{db_12_4_U, imm_8_S }, 32, 739)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("XIY", std::make_unique<machine_instruction>("XIY", mach_format::SIY, std::vector<machine_operand_format>{db_20_4_S, imm_8_S }, 48, 739)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("XC", std::make_unique<machine_instruction>("XC", mach_format::SS_a, std::vector<machine_operand_format>{db_12_8x4L_U, db_20_4_S }, 48, 739)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("EX", std::make_unique<machine_instruction>("EX", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 740)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("XIHF", std::make_unique<machine_instruction>("XIHF", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 740)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("XILF", std::make_unique<machine_instruction>("XILF", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 740)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("EXRL", std::make_unique<machine_instruction>("EXRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 740)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("EAR", std::make_unique<machine_instruction>("EAR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 741)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ECAG", std::make_unique<machine_instruction>("ECAG", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 741)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ECTG", std::make_unique<machine_instruction>("ECTG", mach_format::SSF, std::vector<machine_operand_format>{db_12_4_U, db_12_4_U, reg_4_U }, 48, 744)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("EPSW", std::make_unique<machine_instruction>("EPSW", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 745)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ETND", std::make_unique<machine_instruction>("ETND", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U }, 32, 745)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("FLOGR", std::make_unique<machine_instruction>("FLOGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 746)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("IC", std::make_unique<machine_instruction>("IC", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 746)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ICY", std::make_unique<machine_instruction>("ICY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 746)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ICM", std::make_unique<machine_instruction>("ICM", mach_format::RS_b, std::vector<machine_operand_format>{reg_4_U, mask_4_U, db_12_4_U  }, 32, 746)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ICMY", std::make_unique<machine_instruction>("ICMY", mach_format::RSY_b, std::vector<machine_operand_format>{reg_4_U, mask_4_U, db_20_4_S }, 48, 746)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ICMH", std::make_unique<machine_instruction>("ICMH", mach_format::RSY_b, std::vector<machine_operand_format>{reg_4_U, mask_4_U, db_20_4_S }, 48, 746)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("IIHF", std::make_unique<machine_instruction>("IIHF", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 747)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("IIHH", std::make_unique<machine_instruction>("IIHH", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 747)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("IIHL", std::make_unique<machine_instruction>("IIHL", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 747)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("IILF", std::make_unique<machine_instruction>("IILF", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 747)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("IILH", std::make_unique<machine_instruction>("IILH", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 747)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("IILL", std::make_unique<machine_instruction>("IILL", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 747)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("IPM", std::make_unique<machine_instruction>("IPM", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U }, 32, 748)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LR", std::make_unique<machine_instruction>("LR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 748)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LGR", std::make_unique<machine_instruction>("LGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 748)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LGFR", std::make_unique<machine_instruction>("LGFR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 748)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("L", std::make_unique<machine_instruction>("L", mach_format::RX_a, std::vector<machine_operand_format>{ reg_4_U, dxb_12_4x4_U }, 16, 748)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LY", std::make_unique<machine_instruction>("LY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 16, 748)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LG", std::make_unique<machine_instruction>("LG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 16, 748)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LGF", std::make_unique<machine_instruction>("LGF", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 16, 748)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LGFI", std::make_unique<machine_instruction>("LGFI", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 16, 748)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LRL", std::make_unique<machine_instruction>("LRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 16, 748)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LGRL", std::make_unique<machine_instruction>("LGRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 16, 748)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LGFRL", std::make_unique<machine_instruction>("LGFRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 16, 748)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LAM", std::make_unique<machine_instruction>("LAM", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_12_4_U }, 32, 749)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LAMY", std::make_unique<machine_instruction>("LAMY", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 749)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LA", std::make_unique<machine_instruction>("LA", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 750)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LAY", std::make_unique<machine_instruction>("LAY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 750)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LAE", std::make_unique<machine_instruction>("LAE", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 750)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LAEY", std::make_unique<machine_instruction>("LAEY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 750)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LARL", std::make_unique<machine_instruction>("LARL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 48, 751)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LAA", std::make_unique<machine_instruction>("LAA", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 752)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LAAG", std::make_unique<machine_instruction>("LAAG", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 752)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LAAL", std::make_unique<machine_instruction>("LAAL", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 752)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LAALG", std::make_unique<machine_instruction>("LAALG", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 752)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LAN", std::make_unique<machine_instruction>("LAN", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 753)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LANG", std::make_unique<machine_instruction>("LANG", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 753)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LAX", std::make_unique<machine_instruction>("LAX", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S  }, 48, 753)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LAXG", std::make_unique<machine_instruction>("LAXG", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 753)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LAO", std::make_unique<machine_instruction>("LAO", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 754)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LAOG", std::make_unique<machine_instruction>("LAOG", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 754)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LTR", std::make_unique<machine_instruction>("LTR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 754)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LTGR", std::make_unique<machine_instruction>("LTGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 754)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LTGFR", std::make_unique<machine_instruction>("LTGFR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 754)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LT", std::make_unique<machine_instruction>("LT", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, db_20_4_S }, 48, 755)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LTG", std::make_unique<machine_instruction>("LTG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, db_20_4_S }, 48, 755)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LTGF", std::make_unique<machine_instruction>("LTGF", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, db_20_4_S }, 48, 755)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LAT", std::make_unique<machine_instruction>("LAT", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, db_20_4_S }, 48, 755)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LGAT", std::make_unique<machine_instruction>("LGAT", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, db_20_4_S }, 48, 755)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LZRF", std::make_unique<machine_instruction>("LZRF", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, db_20_4_S }, 48, 755)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LZRG", std::make_unique<machine_instruction>("LZRG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, db_20_4_S }, 48, 755)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LBR", std::make_unique<machine_instruction>("LBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 756)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LGBR", std::make_unique<machine_instruction>("LGBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 756)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LB", std::make_unique<machine_instruction>("LB", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 756)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LGB", std::make_unique<machine_instruction>("LGB", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 756)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LBH", std::make_unique<machine_instruction>("LBH", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 756)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LCR", std::make_unique<machine_instruction>("LCR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 756)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LCGR", std::make_unique<machine_instruction>("LCGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 757)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LCGFR", std::make_unique<machine_instruction>("LCGFR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 757)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LCBB", std::make_unique<machine_instruction>("LCBB", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 757)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LGG", std::make_unique<machine_instruction>("LGG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 758)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLGFSG", std::make_unique<machine_instruction>("LLGFSG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 758)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LGSC", std::make_unique<machine_instruction>("LGSC", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 759)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LHR", std::make_unique<machine_instruction>("LHR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 760)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LGHR", std::make_unique<machine_instruction>("LGHR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 760)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LH", std::make_unique<machine_instruction>("LH", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 760)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LHY", std::make_unique<machine_instruction>("LHY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 760)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LGH", std::make_unique<machine_instruction>("LGH", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 760)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LHI", std::make_unique<machine_instruction>("LHI", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 760)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LGHI", std::make_unique<machine_instruction>("LGHI", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 760)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LHRL", std::make_unique<machine_instruction>("LHRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 760)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LGHRL", std::make_unique<machine_instruction>("LGHRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 760)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LHH", std::make_unique<machine_instruction>("LHH", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 761)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LOCHI", std::make_unique<machine_instruction>("LOCHI", mach_format::RIE_g, std::vector<machine_operand_format>{reg_4_U, imm_16_S , mask_4_U }, 48, 761)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LOCGHI", std::make_unique<machine_instruction>("LOCGHI", mach_format::RIE_g, std::vector<machine_operand_format>{reg_4_U, imm_16_S , mask_4_U }, 48, 761)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LOCHHI", std::make_unique<machine_instruction>("LOCHHI", mach_format::RIE_g, std::vector<machine_operand_format>{reg_4_U, imm_16_S , mask_4_U }, 48, 761)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LFH", std::make_unique<machine_instruction>("LFH", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 762)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LFHAT", std::make_unique<machine_instruction>("LFHAT", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 762)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLGFR", std::make_unique<machine_instruction>("LLGFR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 762)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLGF", std::make_unique<machine_instruction>("LLGF", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 762)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLGFRL", std::make_unique<machine_instruction>("LLGFRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 48, 762)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLGFAT", std::make_unique<machine_instruction>("LLGFAT", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 763)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLCR", std::make_unique<machine_instruction>("LLCR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 763)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLGCR", std::make_unique<machine_instruction>("LLGCR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 763)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLC", std::make_unique<machine_instruction>("LLC", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 763)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLGC", std::make_unique<machine_instruction>("LLGC", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 763)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLZRGF", std::make_unique<machine_instruction>("LLZRGF", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 763)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLCH", std::make_unique<machine_instruction>("LLCH", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 764)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLHR", std::make_unique<machine_instruction>("LLHR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 764)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLGHR", std::make_unique<machine_instruction>("LLGHR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 764)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLH", std::make_unique<machine_instruction>("LLH", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 764)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLGH", std::make_unique<machine_instruction>("LLGH", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 764)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLHRL", std::make_unique<machine_instruction>("LLHRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 48, 764)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLGHRL", std::make_unique<machine_instruction>("LLGHRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 48, 764)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLHH", std::make_unique<machine_instruction>("LLHH", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 765)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLIHF", std::make_unique<machine_instruction>("LLIHF", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 765)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLIHH", std::make_unique<machine_instruction>("LLIHH", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 765)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLIHL", std::make_unique<machine_instruction>("LLIHL", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 765)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLILF", std::make_unique<machine_instruction>("LLILF", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 765)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLILH", std::make_unique<machine_instruction>("LLILH", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 765)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLILL", std::make_unique<machine_instruction>("LLILL", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 765)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLGTR", std::make_unique<machine_instruction>("LLGTR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 765)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLGT", std::make_unique<machine_instruction>("LLGT", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 766)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LLGTAT", std::make_unique<machine_instruction>("LLGTAT", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 766)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LM", std::make_unique<machine_instruction>("LM", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_12_4_U }, 32, 766)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LMY", std::make_unique<machine_instruction>("LMY", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 766)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LMG", std::make_unique<machine_instruction>("LMG", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 766)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LMD", std::make_unique<machine_instruction>("LMD", mach_format::SS_e, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_12_4_U, db_12_4_U }, 48, 767)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LMH", std::make_unique<machine_instruction>("LMH", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 767)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LNR", std::make_unique<machine_instruction>("LNR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 767)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LNGR", std::make_unique<machine_instruction>("LNGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 767)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LNGFR", std::make_unique<machine_instruction>("LNGFR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 768)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LOCFHR", std::make_unique<machine_instruction>("LOCFHR", mach_format::RRF_c, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 32, 768)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LOCFH", std::make_unique<machine_instruction>("LOCFH", mach_format::RSY_b, std::vector<machine_operand_format>{reg_4_U, db_20_4_S, mask_4_U }, 48, 768)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LOCR", std::make_unique<machine_instruction>("LOCR", mach_format::RRF_c, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 32, 768)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LOCGR", std::make_unique<machine_instruction>("LOCGR", mach_format::RRF_c, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 32, 768)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LOC", std::make_unique<machine_instruction>("LOC", mach_format::RSY_b, std::vector<machine_operand_format>{reg_4_U, db_20_4_S, mask_4_U }, 48, 768)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LOCG", std::make_unique<machine_instruction>("LOCG", mach_format::RSY_b, std::vector<machine_operand_format>{reg_4_U, db_20_4_S, mask_4_U }, 48, 768)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LPD", std::make_unique<machine_instruction>("LPD", mach_format::SSF, std::vector<machine_operand_format>{reg_4_U, db_12_4_U, db_12_4_U }, 48, 769)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LPDG", std::make_unique<machine_instruction>("LPDG", mach_format::SSF, std::vector<machine_operand_format>{reg_4_U, db_12_4_U, db_12_4_U }, 48, 769)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LPQ", std::make_unique<machine_instruction>("LPQ", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 770)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LPR", std::make_unique<machine_instruction>("LPR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 771)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LPGR", std::make_unique<machine_instruction>("LPGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 771)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LPGFR", std::make_unique<machine_instruction>("LPGFR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 771)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LRVR", std::make_unique<machine_instruction>("LRVR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 771)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LRVGR", std::make_unique<machine_instruction>("LRVGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 771)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LRVH", std::make_unique<machine_instruction>("LRVH", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 32, 771)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LRV", std::make_unique<machine_instruction>("LRV", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 32, 771)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LRVG", std::make_unique<machine_instruction>("LRVG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 32, 771)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MC", std::make_unique<machine_instruction>("MC", mach_format::SI, std::vector<machine_operand_format>{db_12_4_U, imm_8_S }, 32, 772)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MVC", std::make_unique<machine_instruction>("MVC", mach_format::SS_a, std::vector<machine_operand_format>{db_12_8x4L_U, db_12_4_U }, 48, 773)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MVHHI", std::make_unique<machine_instruction>("MVHHI", mach_format::SIL, std::vector<machine_operand_format>{db_12_4_U, imm_16_S  }, 48, 773)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MVHI", std::make_unique<machine_instruction>("MVHI", mach_format::SIL, std::vector<machine_operand_format>{db_12_4_U, imm_16_S  }, 48, 773)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MVGHI", std::make_unique<machine_instruction>("MVGHI", mach_format::SIL, std::vector<machine_operand_format>{db_12_4_U, imm_16_S  }, 48, 773)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MVI", std::make_unique<machine_instruction>("MVI", mach_format::SI, std::vector<machine_operand_format>{db_12_4_U, imm_8_S }, 32, 773)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MVIY", std::make_unique<machine_instruction>("MVIY", mach_format::SIY, std::vector<machine_operand_format>{db_12_4_U, imm_8_S }, 48, 773)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MVCIN", std::make_unique<machine_instruction>("MVCIN", mach_format::SS_a, std::vector<machine_operand_format>{ db_12_8x4L_U, db_12_4_U }, 48, 774)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MVCL", std::make_unique<machine_instruction>("MVCL", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 774)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MVCLE", std::make_unique<machine_instruction>("MVCLE", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_12_4_U }, 32, 778)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MVCLU", std::make_unique<machine_instruction>("MVCLU", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 781)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MVN", std::make_unique<machine_instruction>("MVN", mach_format::SS_a, std::vector<machine_operand_format>{ db_12_8x4L_U, db_12_4_U }, 48, 785)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MVST", std::make_unique<machine_instruction>("MVST", mach_format::RRE, std::vector<machine_operand_format>{ reg_4_U, reg_4_U }, 32, 785)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MVO", std::make_unique<machine_instruction>("MVO", mach_format::SS_b, std::vector<machine_operand_format>{ db_12_4x4L_U, db_12_4x4L_U }, 48, 786)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MVZ", std::make_unique<machine_instruction>("MVZ", mach_format::SS_a, std::vector<machine_operand_format>{db_12_4x4L_U, db_12_4_U }, 48, 787)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MR", std::make_unique<machine_instruction>("MR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 788)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MGRK", std::make_unique<machine_instruction>("MGRK", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 788)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("M", std::make_unique<machine_instruction>("M", mach_format::RX_a, std::vector<machine_operand_format>{ reg_4_U, dxb_12_4x4_U }, 32, 788)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MFY", std::make_unique<machine_instruction>("MFY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 788)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MG", std::make_unique<machine_instruction>("MG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 788)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MH", std::make_unique<machine_instruction>("MH", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 789)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MHY", std::make_unique<machine_instruction>("MHY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 789)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MGH", std::make_unique<machine_instruction>("MGH", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 789)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MHI", std::make_unique<machine_instruction>("MHI", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 789)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MGHI", std::make_unique<machine_instruction>("MGHI", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 789)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MLR", std::make_unique<machine_instruction>("MLR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 790)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MLGR", std::make_unique<machine_instruction>("MLGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 790)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ML", std::make_unique<machine_instruction>("ML", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 790)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MLG", std::make_unique<machine_instruction>("MLG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 790)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSR", std::make_unique<machine_instruction>("MSR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 791)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSRKC", std::make_unique<machine_instruction>("MSRKC", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 791)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSGR", std::make_unique<machine_instruction>("MSGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 791)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSGRKC", std::make_unique<machine_instruction>("MSGRKC", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 791)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSGFR", std::make_unique<machine_instruction>("MSGFR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 791)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MS", std::make_unique<machine_instruction>("MS", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 791)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSC", std::make_unique<machine_instruction>("MSC", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 791)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSY", std::make_unique<machine_instruction>("MSY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 791)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSG", std::make_unique<machine_instruction>("MSG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 791)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSGC", std::make_unique<machine_instruction>("MSGC", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 791)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSGF", std::make_unique<machine_instruction>("MSGF", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 791)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSFI", std::make_unique<machine_instruction>("MSFI", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 791)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSGFI", std::make_unique<machine_instruction>("MSGFI", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 791)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("NIAI", std::make_unique<machine_instruction>("NIAI", mach_format::IE, std::vector<machine_operand_format>{imm_4_U, imm_4_U }, 32, 792)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("NTSTG", std::make_unique<machine_instruction>("NTSTG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 794)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("OR", std::make_unique<machine_instruction>("OR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 794)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("OGR", std::make_unique<machine_instruction>("OGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 794)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ORK", std::make_unique<machine_instruction>("ORK", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 794)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("OGRK", std::make_unique<machine_instruction>("OGRK", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 794)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("O", std::make_unique<machine_instruction>("O", mach_format::RX_a, std::vector<machine_operand_format>{ reg_4_U, dxb_12_4x4_U }, 32, 794)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("OY", std::make_unique<machine_instruction>("OY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 32, 794)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("OG", std::make_unique<machine_instruction>("OG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 795)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("OI", std::make_unique<machine_instruction>("OI", mach_format::SI, std::vector<machine_operand_format>{db_12_4_U, imm_8_S }, 48, 795)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("OIY", std::make_unique<machine_instruction>("OIY", mach_format::SIY, std::vector<machine_operand_format>{ db_20_4_S, imm_8_S }, 48, 795)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("OC", std::make_unique<machine_instruction>("OC", mach_format::SS_a, std::vector<machine_operand_format>{ db_12_8x4L_U, db_12_4_U }, 48, 795)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("OIHF", std::make_unique<machine_instruction>("OIHF", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 32, 796)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("OIHH", std::make_unique<machine_instruction>("OIHH", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 796)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("OIHL", std::make_unique<machine_instruction>("OIHL", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 796)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("OILF", std::make_unique<machine_instruction>("OILF", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 32, 796)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("OILH", std::make_unique<machine_instruction>("OILH", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 796)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("OILL", std::make_unique<machine_instruction>("OILL", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 796)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PACK", std::make_unique<machine_instruction>("PACK", mach_format::SS_b, std::vector<machine_operand_format>{ db_12_4x4L_U, db_12_4x4L_U }, 32, 796)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PKA", std::make_unique<machine_instruction>("PKA", mach_format::SS_f, std::vector<machine_operand_format>{db_12_4_U, db_12_8x4L_U }, 48, 797)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PKU", std::make_unique<machine_instruction>("PKU", mach_format::SS_f, std::vector<machine_operand_format>{db_12_4_U, db_12_8x4L_U }, 48, 798)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PCC", std::make_unique<machine_instruction>("PCC", mach_format::RRE, std::vector<machine_operand_format>{ }, 32, 799)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PLO", std::make_unique<machine_instruction>("PLO", mach_format::SS_e, std::vector<machine_operand_format>{reg_4_U, db_12_4_U, reg_4_U, db_12_4_U }, 48, 815)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PPA", std::make_unique<machine_instruction>("PPA", mach_format::RRF_c, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 32, 829)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PRNO", std::make_unique<machine_instruction>("PRNO", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 830)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PPNO", std::make_unique<machine_instruction>("PPNO", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 830)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("POPCNT", std::make_unique<machine_instruction>("POPCNT", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 48, 843)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PFD", std::make_unique<machine_instruction>("PFD", mach_format::RXY_b, std::vector<machine_operand_format>{mask_4_U, dxb_20_4x4_S }, 48, 843)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PFDRL", std::make_unique<machine_instruction>("PFDRL", mach_format::RIL_c, std::vector<machine_operand_format>{mask_4_U, reg_imm_32_S }, 48, 843)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("RLL", std::make_unique<machine_instruction>("RLL", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 845)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("RLLG", std::make_unique<machine_instruction>("RLLG", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 845)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("RNSBG", std::make_unique<machine_instruction>("RNSBG", mach_format::RIE_f, std::vector<machine_operand_format>{reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S }, 1, 48, 845)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("RXSBG", std::make_unique<machine_instruction>("RXSBG", mach_format::RIE_f, std::vector<machine_operand_format>{reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S }, 1, 48, 846)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ROSBG", std::make_unique<machine_instruction>("ROSBG", mach_format::RIE_f, std::vector<machine_operand_format>{reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S }, 1, 48, 846)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("RISBG", std::make_unique<machine_instruction>("RISBG", mach_format::RIE_f, std::vector<machine_operand_format>{reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S }, 1, 48, 847)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("RISBGN", std::make_unique<machine_instruction>("RISBGN", mach_format::RIE_f, std::vector<machine_operand_format>{reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S }, 1, 48, 847)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("RISBHG", std::make_unique<machine_instruction>("RISBHG", mach_format::RIE_f, std::vector<machine_operand_format>{reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S  }, 1, 48, 848)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("RISBLG", std::make_unique<machine_instruction>("RISBLG", mach_format::RIE_f, std::vector<machine_operand_format>{reg_4_U, reg_4_U, imm_8_S, imm_8_S, imm_8_S  }, 1, 48, 849)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SRST", std::make_unique<machine_instruction>("SRST", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 850)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SRSTU", std::make_unique<machine_instruction>("SRSTU", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 852)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SAR", std::make_unique<machine_instruction>("SAR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 854)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SAM24", std::make_unique<machine_instruction>("SAM24", mach_format::E, std::vector<machine_operand_format>{ }, 16, 854)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SAM31", std::make_unique<machine_instruction>("SAM31", mach_format::E, std::vector<machine_operand_format>{ }, 16, 854)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SAM64", std::make_unique<machine_instruction>("SAM64", mach_format::E, std::vector<machine_operand_format>{ }, 16, 854)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SPM", std::make_unique<machine_instruction>("SPM", mach_format::RR, std::vector<machine_operand_format>{reg_4_U }, 16, 855)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLDA", std::make_unique<machine_instruction>("SLDA", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, db_12_4_U }, 32, 855)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLA", std::make_unique<machine_instruction>("SLA", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, db_12_4_U }, 32, 856)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLAK", std::make_unique<machine_instruction>("SLAK", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 856)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLAG", std::make_unique<machine_instruction>("SLAG", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 856)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLDL", std::make_unique<machine_instruction>("SLDL", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, db_12_4_U }, 32, 856)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLL", std::make_unique<machine_instruction>("SLL", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, db_12_4_U }, 32, 857)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLLK", std::make_unique<machine_instruction>("SLLK", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 857)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLLG", std::make_unique<machine_instruction>("SLLG", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 857)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SRDA", std::make_unique<machine_instruction>("SRDA", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, db_12_4_U }, 32, 858)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SRDL", std::make_unique<machine_instruction>("SRDL", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, db_12_4_U }, 32, 858)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SRA", std::make_unique<machine_instruction>("SRA", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, db_12_4_U }, 32, 859)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SRAK", std::make_unique<machine_instruction>("SRAK", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 859)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SRAG", std::make_unique<machine_instruction>("SRAG", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 859)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SRL", std::make_unique<machine_instruction>("SRL", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, db_12_4_U }, 32, 860)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SRLK", std::make_unique<machine_instruction>("SRLK", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 860)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SRLG", std::make_unique<machine_instruction>("SRLG", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 860)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ST", std::make_unique<machine_instruction>("ST", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 860)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STY", std::make_unique<machine_instruction>("STY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 861)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STG", std::make_unique<machine_instruction>("STG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 861)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STRL", std::make_unique<machine_instruction>("STRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 48, 861)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STGRL", std::make_unique<machine_instruction>("STGRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 48, 861)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STAM", std::make_unique<machine_instruction>("STAM", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_12_4_U }, 48, 861)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STAMY", std::make_unique<machine_instruction>("STAMY", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 861)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STC", std::make_unique<machine_instruction>("STC", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 862)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STCY", std::make_unique<machine_instruction>("STCY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 862)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STCH", std::make_unique<machine_instruction>("STCH", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 862)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STCM", std::make_unique<machine_instruction>("STCM", mach_format::RS_b, std::vector<machine_operand_format>{reg_4_U, mask_4_U, db_12_4_U }, 32, 862)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STCMY", std::make_unique<machine_instruction>("STCMY", mach_format::RSY_b, std::vector<machine_operand_format>{reg_4_U, mask_4_U, db_20_4_S }, 48, 862)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STCMH", std::make_unique<machine_instruction>("STCMH", mach_format::RSY_b, std::vector<machine_operand_format>{reg_4_U, mask_4_U, db_20_4_S }, 48, 862)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STCK", std::make_unique<machine_instruction>("STCK", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 863)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STCKF", std::make_unique<machine_instruction>("STCKF", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 863)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STCKE", std::make_unique<machine_instruction>("STCKE", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 864)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STFLE", std::make_unique<machine_instruction>("STFLE", mach_format::S, std::vector<machine_operand_format>{ db_20_4_S }, 48, 866)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STGSC", std::make_unique<machine_instruction>("STGSC", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 32, 867)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STH", std::make_unique<machine_instruction>("STH", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 867)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STHY", std::make_unique<machine_instruction>("STHY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 868)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STHRL", std::make_unique<machine_instruction>("STHRL", mach_format::RIL_b, std::vector<machine_operand_format>{reg_4_U, reg_imm_32_S }, 48, 868)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STHH", std::make_unique<machine_instruction>("STHH", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 868)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STFH", std::make_unique<machine_instruction>("STFH", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 868)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STM", std::make_unique<machine_instruction>("STM", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_12_4_U }, 32, 869)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STMY", std::make_unique<machine_instruction>("STMY", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 869)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STMG", std::make_unique<machine_instruction>("STMG", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 869)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STMH", std::make_unique<machine_instruction>("STMH", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 869)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STOC", std::make_unique<machine_instruction>("STOC", mach_format::RSY_b, std::vector<machine_operand_format>{reg_4_U, db_20_4_S, mask_4_U }, 48, 869)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STOCG", std::make_unique<machine_instruction>("STOCG", mach_format::RSY_b, std::vector<machine_operand_format>{reg_4_U, db_20_4_S, mask_4_U }, 48, 869)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STOCFH", std::make_unique<machine_instruction>("STOCFH", mach_format::RSY_b, std::vector<machine_operand_format>{reg_4_U, db_20_4_S, mask_4_U }, 48, 870)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STPQ", std::make_unique<machine_instruction>("STPQ", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 870)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STRVH", std::make_unique<machine_instruction>("STRVH", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 871)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STRV", std::make_unique<machine_instruction>("STRV", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 871)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STRVG", std::make_unique<machine_instruction>("STRVG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 871)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SR", std::make_unique<machine_instruction>("SR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 871)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SGR", std::make_unique<machine_instruction>("SGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 871)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SGFR", std::make_unique<machine_instruction>("SGFR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 871)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SRK", std::make_unique<machine_instruction>("SRK", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 871)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SGRK", std::make_unique<machine_instruction>("SGRK", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 872)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("S", std::make_unique<machine_instruction>("S", mach_format::RX_a, std::vector<machine_operand_format>{ reg_4_U, dxb_12_4x4_U}, 32, 872)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SY", std::make_unique<machine_instruction>("SY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 872)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SG", std::make_unique<machine_instruction>("SG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 872)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SGF", std::make_unique<machine_instruction>("SGF", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 872)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SH", std::make_unique<machine_instruction>("SH", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 872)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SHY", std::make_unique<machine_instruction>("SHY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 872)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SGH", std::make_unique<machine_instruction>("SGH", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 872)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SHHHR", std::make_unique<machine_instruction>("SHHHR", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 873)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SHHLR", std::make_unique<machine_instruction>("SHHLR", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 873)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLR", std::make_unique<machine_instruction>("SLR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 873)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLGR", std::make_unique<machine_instruction>("SLGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 873)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLGFR", std::make_unique<machine_instruction>("SLGFR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 873)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLRK", std::make_unique<machine_instruction>("SLRK", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 873)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLGRK", std::make_unique<machine_instruction>("SLGRK", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 873)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SL", std::make_unique<machine_instruction>("SL", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 874)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLY", std::make_unique<machine_instruction>("SLY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 874)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLG", std::make_unique<machine_instruction>("SLG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 874)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLGF", std::make_unique<machine_instruction>("SLGF", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 874)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLFI", std::make_unique<machine_instruction>("SLFI", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 874)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLGFI", std::make_unique<machine_instruction>("SLGFI", mach_format::RIL_a, std::vector<machine_operand_format>{reg_4_U, imm_32_S }, 48, 874)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLHHHR", std::make_unique<machine_instruction>("SLHHHR", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 875)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLHHLR", std::make_unique<machine_instruction>("SLHHLR", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 875)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLBR", std::make_unique<machine_instruction>("SLBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 875)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLBGR", std::make_unique<machine_instruction>("SLBGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 875)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLB", std::make_unique<machine_instruction>("SLB", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 875)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLBG", std::make_unique<machine_instruction>("SLBG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 875)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SVC", std::make_unique<machine_instruction>("SVC", mach_format::I, std::vector<machine_operand_format>{imm_8_U }, 16, 876)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TS", std::make_unique<machine_instruction>("TS", mach_format::SI, std::vector<machine_operand_format>{db_12_4_U }, 32, 876)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TAM", std::make_unique<machine_instruction>("TAM", mach_format::E, std::vector<machine_operand_format>{ }, 16, 876)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TM", std::make_unique<machine_instruction>("TM", mach_format::SI, std::vector<machine_operand_format>{db_12_4_U, imm_8_S }, 32, 877)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TMY", std::make_unique<machine_instruction>("TMY", mach_format::SIY, std::vector<machine_operand_format>{db_20_4_S, imm_8_S }, 48, 877)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TMHH", std::make_unique<machine_instruction>("TMHH", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 877)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TMHL", std::make_unique<machine_instruction>("TMHL", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 877)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TMH", std::make_unique<machine_instruction>("TMH", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 877)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TMLH", std::make_unique<machine_instruction>("TMLH", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 877)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TML", std::make_unique<machine_instruction>("TML", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 877)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TMLL", std::make_unique<machine_instruction>("TMLL", mach_format::RI_a, std::vector<machine_operand_format>{reg_4_U, imm_16_S  }, 32, 877)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TABORT", std::make_unique<machine_instruction>("TABORT", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 878)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TBEGIN", std::make_unique<machine_instruction>("TBEGIN", mach_format::SIL, std::vector<machine_operand_format>{db_12_4_U, imm_16_S  }, 48, 879)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TBEGINC", std::make_unique<machine_instruction>("TBEGINC", mach_format::SIL, std::vector<machine_operand_format>{db_12_4_U, imm_16_S  }, 48, 883)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TEND", std::make_unique<machine_instruction>("TEND", mach_format::S, std::vector<machine_operand_format>{ }, 32, 885)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TR", std::make_unique<machine_instruction>("TR", mach_format::SS_a, std::vector<machine_operand_format>{ db_12_8x4L_U, db_12_4_U }, 48, 886)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TRT", std::make_unique<machine_instruction>("TRT", mach_format::SS_a, std::vector<machine_operand_format>{ db_12_8x4L_U, db_12_4_U }, 48, 887)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TRTE", std::make_unique<machine_instruction>("TRTE", mach_format::RRF_c, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 1, 32, 887)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TRTRE", std::make_unique<machine_instruction>("TRTRE", mach_format::RRF_c, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 1, 32, 888)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TRTR", std::make_unique<machine_instruction>("TRTR", mach_format::SS_a, std::vector<machine_operand_format>{ db_12_8x4L_U, db_12_4_U }, 48, 892)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TRE", std::make_unique<machine_instruction>("TRE", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 893)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TROO", std::make_unique<machine_instruction>("TROO", mach_format::RRF_c, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 1, 32, 895)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TROT", std::make_unique<machine_instruction>("TROT", mach_format::RRF_c, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 1, 32, 895)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TRTO", std::make_unique<machine_instruction>("TRTO", mach_format::RRF_c, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 1, 32, 895)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TRTT", std::make_unique<machine_instruction>("TRTT", mach_format::RRF_c, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 1, 32, 895)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("UNPK", std::make_unique<machine_instruction>("UNPK", mach_format::SS_b, std::vector<machine_operand_format>{ db_12_8x4L_U, db_12_4_U }, 48, 900)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("UNPKA", std::make_unique<machine_instruction>("UNPKA", mach_format::SS_a, std::vector<machine_operand_format>{ db_12_8x4L_U, db_12_4_U }, 48, 901)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("UNPKU", std::make_unique<machine_instruction>("UNPKU", mach_format::SS_a, std::vector<machine_operand_format>{ db_12_8x4L_U, db_12_4_U }, 48, 902)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("UPT", std::make_unique<machine_instruction>("UPT", mach_format::E, std::vector<machine_operand_format>{ }, 16, 903)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AP", std::make_unique<machine_instruction>("AP", mach_format::SS_b, std::vector<machine_operand_format>{ db_12_4x4L_U, db_12_4x4L_U }, 48, 920)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CP", std::make_unique<machine_instruction>("CP", mach_format::SS_b, std::vector<machine_operand_format>{ db_12_4x4L_U, db_12_4x4L_U }, 48, 921)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DP", std::make_unique<machine_instruction>("DP", mach_format::SS_b, std::vector<machine_operand_format>{ db_12_4x4L_U, db_12_4x4L_U }, 48, 921)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ED", std::make_unique<machine_instruction>("ED", mach_format::SS_a, std::vector<machine_operand_format>{ db_12_8x4L_U, db_12_4_U }, 48, 922)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("EDMK", std::make_unique<machine_instruction>("EDMK", mach_format::SS_a, std::vector<machine_operand_format>{ db_12_8x4L_U, db_12_4_U }, 48, 925)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SRP", std::make_unique<machine_instruction>("SRP", mach_format::SS_c, std::vector<machine_operand_format>{ db_12_4x4L_U, db_12_4_U, imm_4_U }, 48, 926)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MP", std::make_unique<machine_instruction>("MP", mach_format::SS_b, std::vector<machine_operand_format>{ db_12_4x4L_U, db_12_4x4L_U }, 48, 926)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SP", std::make_unique<machine_instruction>("SP", mach_format::SS_b, std::vector<machine_operand_format>{ db_12_4x4L_U, db_12_4x4L_U }, 48, 927)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TP", std::make_unique<machine_instruction>("TP", mach_format::RSL_a, std::vector<machine_operand_format>{ db_12_4x4L_U }, 48, 928)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ZAP", std::make_unique<machine_instruction>("ZAP", mach_format::SS_b, std::vector<machine_operand_format>{ db_12_4x4L_U, db_12_4x4L_U }, 48, 928)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("THDR", std::make_unique<machine_instruction>("THDR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 955)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("THDER", std::make_unique<machine_instruction>("THDER", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 955)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TBEDR", std::make_unique<machine_instruction>("TBEDR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U }, 32, 956)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TBDR", std::make_unique<machine_instruction>("TBDR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U }, 32, 956)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CPSDR", std::make_unique<machine_instruction>("CPSDR", mach_format::RRF_b, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 958)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("EFPC", std::make_unique<machine_instruction>("EFPC", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U }, 32, 958)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LER", std::make_unique<machine_instruction>("LER", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 959)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LDR", std::make_unique<machine_instruction>("LDR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 959)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LXR", std::make_unique<machine_instruction>("LXR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 959)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LE", std::make_unique<machine_instruction>("LE", mach_format::RX_a, std::vector<machine_operand_format>{ reg_4_U, dxb_12_4x4_U }, 32, 959)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LD", std::make_unique<machine_instruction>("LD", mach_format::RX_a, std::vector<machine_operand_format>{ reg_4_U, dxb_12_4x4_U }, 32, 959)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LEY", std::make_unique<machine_instruction>("LEY", mach_format::RXY_a, std::vector<machine_operand_format>{ reg_4_U, dxb_20_4x4_S }, 48, 959)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LDY", std::make_unique<machine_instruction>("LDY", mach_format::RXY_a, std::vector<machine_operand_format>{ reg_4_U, dxb_20_4x4_S }, 48, 959)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LCDFR", std::make_unique<machine_instruction>("LCDFR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 959)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LFPC", std::make_unique<machine_instruction>("LFPC", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 959)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LFAS", std::make_unique<machine_instruction>("LFAS", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 960)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LDGR", std::make_unique<machine_instruction>("LDGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 962)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LGDR", std::make_unique<machine_instruction>("LGDR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 962)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LNDFR", std::make_unique<machine_instruction>("LNDFR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 962)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LPDFR", std::make_unique<machine_instruction>("LPDFR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 962)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LZER", std::make_unique<machine_instruction>("LZER", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U }, 32, 963)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LZXR", std::make_unique<machine_instruction>("LZXR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U }, 32, 963)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LZDR", std::make_unique<machine_instruction>("LZDR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U }, 32, 963)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PFPO", std::make_unique<machine_instruction>("PFPO", mach_format::E, std::vector<machine_operand_format>{ }, 16, 963)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SRNM", std::make_unique<machine_instruction>("SRNM", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 975)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SRNMB", std::make_unique<machine_instruction>("SRNMB", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 975)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SRNMT", std::make_unique<machine_instruction>("SRNMT", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 975)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SFPC", std::make_unique<machine_instruction>("SFPC", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U }, 32, 975)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SFASR", std::make_unique<machine_instruction>("SFASR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U }, 32, 976)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STE", std::make_unique<machine_instruction>("STE", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 976)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STD", std::make_unique<machine_instruction>("STD", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 976)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STDY", std::make_unique<machine_instruction>("STDY", mach_format::RXY_a, std::vector<machine_operand_format>{ reg_4_U, dxb_20_4x4_S }, 48, 977)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STEY", std::make_unique<machine_instruction>("STEY", mach_format::RXY_a, std::vector<machine_operand_format>{ reg_4_U, dxb_20_4x4_S }, 48, 977)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STFPC", std::make_unique<machine_instruction>("STFPC", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 977)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BSA", std::make_unique<machine_instruction>("BSA", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 989)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BAKR", std::make_unique<machine_instruction>("BAKR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 993)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("BSG", std::make_unique<machine_instruction>("BSG", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 995)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CRDTE", std::make_unique<machine_instruction>("CRDTE", mach_format::RRF_b, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1, 32, 999)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CSP", std::make_unique<machine_instruction>("CSP", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1003)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CSPG", std::make_unique<machine_instruction>("CSPG", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1003)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ESEA", std::make_unique<machine_instruction>("ESEA", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U }, 32, 1006)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("EPAR", std::make_unique<machine_instruction>("EPAR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U }, 32, 1006)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("EPAIR", std::make_unique<machine_instruction>("EPAIR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U }, 32, 1006)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ESAR", std::make_unique<machine_instruction>("ESAR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U }, 32, 1006)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ESAIR", std::make_unique<machine_instruction>("ESAIR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U }, 32, 1007)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("EREG", std::make_unique<machine_instruction>("EREG", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1007)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("EREGG", std::make_unique<machine_instruction>("EREGG", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1007)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ESTA", std::make_unique<machine_instruction>("ESTA", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1008)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("IAC", std::make_unique<machine_instruction>("IAC", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U }, 32, 1011)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("IPK", std::make_unique<machine_instruction>("IPK", mach_format::S, std::vector<machine_operand_format>{ }, 32, 1012)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("IRBM", std::make_unique<machine_instruction>("IRBM", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1012)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ISKE", std::make_unique<machine_instruction>("ISKE", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1012)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("IVSK", std::make_unique<machine_instruction>("IVSK", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1013)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("IDTE", std::make_unique<machine_instruction>("IDTE", mach_format::RRF_b, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 1, 32, 1014)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("IPTE", std::make_unique<machine_instruction>("IPTE", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 2, 32, 1019)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LASP", std::make_unique<machine_instruction>("LASP", mach_format::SSE, std::vector<machine_operand_format>{db_12_4_U, db_12_4_U }, 48, 1023)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LCTL", std::make_unique<machine_instruction>("LCTL", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_12_4_U }, 32, 1032)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LCTLG", std::make_unique<machine_instruction>("LCTLG", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 1032)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LPTEA", std::make_unique<machine_instruction>("LPTEA", mach_format::RRF_b, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1032)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LPSW", std::make_unique<machine_instruction>("LPSW", mach_format::SI, std::vector<machine_operand_format>{db_12_4_U }, 32, 1036)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LPSWE", std::make_unique<machine_instruction>("LPSWE", mach_format::S, std::vector<machine_operand_format>{ db_12_4_U }, 32, 1037)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LRA", std::make_unique<machine_instruction>("LRA", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 1038)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LRAY", std::make_unique<machine_instruction>("LRAY", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 1038)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LRAG", std::make_unique<machine_instruction>("LRAG", mach_format::RXY_a, std::vector<machine_operand_format>{reg_4_U, dxb_20_4x4_S }, 48, 1038)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LURA", std::make_unique<machine_instruction>("LURA", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1042)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LURAG", std::make_unique<machine_instruction>("LURAG", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1042)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSTA", std::make_unique<machine_instruction>("MSTA", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U }, 32, 1043)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MVPG", std::make_unique<machine_instruction>("MVPG", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1044)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MVCP", std::make_unique<machine_instruction>("MVCP", mach_format::SS_d, std::vector<machine_operand_format>{drb_12_4x4_U, db_12_4_U, reg_4_U }, 48, 1046)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MVCS", std::make_unique<machine_instruction>("MVCS", mach_format::SS_d, std::vector<machine_operand_format>{drb_12_4x4_U, db_12_4_U, reg_4_U }, 48, 1046)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MVCDK", std::make_unique<machine_instruction>("MVCDK", mach_format::SSE, std::vector<machine_operand_format>{db_12_4_U, db_12_4_U }, 48, 1048)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MVCK", std::make_unique<machine_instruction>("MVCK", mach_format::SS_d, std::vector<machine_operand_format>{drb_12_4x4_U, db_12_4_U, reg_4_U }, 48, 1049)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MVCOS", std::make_unique<machine_instruction>("MVCOS", mach_format::SSF, std::vector<machine_operand_format>{db_12_4_U, db_12_4_U, reg_4_U }, 48, 1050)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MVCSK", std::make_unique<machine_instruction>("MVCSK", mach_format::SSE, std::vector<machine_operand_format>{db_12_4_U, db_12_4_U }, 48, 1053)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PGIN", std::make_unique<machine_instruction>("PGIN", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1054)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PGOUT", std::make_unique<machine_instruction>("PGOUT", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1055)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PCKMO", std::make_unique<machine_instruction>("PCKMO", mach_format::RRE, std::vector<machine_operand_format>{ }, 32, 1056)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PFMF", std::make_unique<machine_instruction>("PFMF", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1059)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PTFF", std::make_unique<machine_instruction>("PTFF", mach_format::E, std::vector<machine_operand_format>{ }, 16, 1063)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PTF", std::make_unique<machine_instruction>("PTF", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U }, 32, 1071)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PC", std::make_unique<machine_instruction>("PC", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1072)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PR", std::make_unique<machine_instruction>("PR", mach_format::E, std::vector<machine_operand_format>{ }, 16, 1085)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PTI", std::make_unique<machine_instruction>("PTI", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1089)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PT", std::make_unique<machine_instruction>("PT", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1089)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PALB", std::make_unique<machine_instruction>("PALB", mach_format::RRE, std::vector<machine_operand_format>{ }, 32, 1098)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("PTLB", std::make_unique<machine_instruction>("PTLB", mach_format::S, std::vector<machine_operand_format>{ }, 32, 1098)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("RRBE", std::make_unique<machine_instruction>("RRBE", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1098)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("RRBM", std::make_unique<machine_instruction>("RRBM", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1099)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("RP", std::make_unique<machine_instruction>("RP", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1099)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SAC", std::make_unique<machine_instruction>("SAC", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1102)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SACF", std::make_unique<machine_instruction>("SACF", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1102)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SCK", std::make_unique<machine_instruction>("SCK", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1103)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SCKC", std::make_unique<machine_instruction>("SCKC", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1104)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SCKPF", std::make_unique<machine_instruction>("SCKPF", mach_format::E, std::vector<machine_operand_format>{ }, 16, 1105)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SPX", std::make_unique<machine_instruction>("SPX", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1105)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SPT", std::make_unique<machine_instruction>("SPT", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1105)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SPKA", std::make_unique<machine_instruction>("SPKA", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1106)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SSAR", std::make_unique<machine_instruction>("SSAR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U }, 32, 1107)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SSAIR", std::make_unique<machine_instruction>("SSAIR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U }, 32, 1107)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SSKE", std::make_unique<machine_instruction>("SSKE", mach_format::RRF_c, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 1, 32, 1112)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SSM", std::make_unique<machine_instruction>("SSM", mach_format::SI, std::vector<machine_operand_format>{db_12_4_U }, 32, 1115)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SIGP", std::make_unique<machine_instruction>("SIGP", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_12_4_U }, 32, 1115)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STCKC", std::make_unique<machine_instruction>("STCKC", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1117)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STCTL", std::make_unique<machine_instruction>("STCTL", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_12_4_U }, 32, 1117)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STCTG", std::make_unique<machine_instruction>("STCTG", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 1117)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STAP", std::make_unique<machine_instruction>("STAP", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1118)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STIDP", std::make_unique<machine_instruction>("STIDP", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1118)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STPT", std::make_unique<machine_instruction>("STPT", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1120)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STFL", std::make_unique<machine_instruction>("STFL", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1120)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STPX", std::make_unique<machine_instruction>("STPX", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1121)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STRAG", std::make_unique<machine_instruction>("STRAG", mach_format::SSE, std::vector<machine_operand_format>{db_12_4_U, db_12_4_U }, 32, 1121)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STSI", std::make_unique<machine_instruction>("STSI", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1122)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STOSM", std::make_unique<machine_instruction>("STOSM", mach_format::SI, std::vector<machine_operand_format>{db_12_4_U, imm_8_S }, 32, 1146)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STNSM", std::make_unique<machine_instruction>("STNSM", mach_format::SI, std::vector<machine_operand_format>{db_12_4_U, imm_8_S }, 32, 1146)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STURA", std::make_unique<machine_instruction>("STURA", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1147)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STURG", std::make_unique<machine_instruction>("STURG", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1147)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TAR", std::make_unique<machine_instruction>("TAR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1147)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TB", std::make_unique<machine_instruction>("TB", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1149)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TPEI", std::make_unique<machine_instruction>("TPEI", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1151)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TPROT", std::make_unique<machine_instruction>("TPROT", mach_format::SSE, std::vector<machine_operand_format>{db_12_4_U, db_12_4_U }, 48, 1152)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TRACE", std::make_unique<machine_instruction>("TRACE", mach_format::RS_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_12_4_U }, 32, 1155)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TRACG", std::make_unique<machine_instruction>("TRACG", mach_format::RSY_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, db_20_4_S }, 48, 1155)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TRAP2", std::make_unique<machine_instruction>("TRAP2", mach_format::E, std::vector<machine_operand_format>{ }, 16, 1156)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TRAP4", std::make_unique<machine_instruction>("TRAP4", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1156)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("XSCH", std::make_unique<machine_instruction>("XSCH", mach_format::S, std::vector<machine_operand_format>{ }, 32, 1215)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CSCH", std::make_unique<machine_instruction>("CSCH", mach_format::S, std::vector<machine_operand_format>{ }, 32, 1217)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("HSCH", std::make_unique<machine_instruction>("HSCH", mach_format::S, std::vector<machine_operand_format>{ }, 32, 1218)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSCH", std::make_unique<machine_instruction>("MSCH", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1219)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("RCHP", std::make_unique<machine_instruction>("RCHP", mach_format::S, std::vector<machine_operand_format>{ }, 32, 1221)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("RSCH", std::make_unique<machine_instruction>("RSCH", mach_format::S, std::vector<machine_operand_format>{ }, 32, 1222)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SAL", std::make_unique<machine_instruction>("SAL", mach_format::S, std::vector<machine_operand_format>{ }, 32, 1224)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SCHM", std::make_unique<machine_instruction>("SCHM", mach_format::S, std::vector<machine_operand_format>{ }, 32, 1225)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SSCH", std::make_unique<machine_instruction>("SSCH", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1227)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STCPS", std::make_unique<machine_instruction>("STCPS", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1228)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STCRW", std::make_unique<machine_instruction>("STCRW", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1229)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("STSCH", std::make_unique<machine_instruction>("STSCH", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1230)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TPI", std::make_unique<machine_instruction>("TPI", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1231)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TSCH", std::make_unique<machine_instruction>("TSCH", mach_format::S, std::vector<machine_operand_format>{db_12_4_U }, 32, 1232)));
	// start of mnemonics
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AER", std::make_unique<machine_instruction>("AER", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1412)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ADR", std::make_unique<machine_instruction>("ADR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1412)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AXR", std::make_unique<machine_instruction>("AXR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1412)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AE", std::make_unique<machine_instruction>("AE", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 1412)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AD", std::make_unique<machine_instruction>("AD", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 1412)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AWR", std::make_unique<machine_instruction>("AWR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1413)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AUR", std::make_unique<machine_instruction>("AUR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1413)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AU", std::make_unique<machine_instruction>("AU", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 1413)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AW", std::make_unique<machine_instruction>("AW", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 1413)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CER", std::make_unique<machine_instruction>("CER", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1414)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDR", std::make_unique<machine_instruction>("CDR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1414)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CXR", std::make_unique<machine_instruction>("CXR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1414)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CE", std::make_unique<machine_instruction>("CE", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 1414)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CD", std::make_unique<machine_instruction>("CD", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 1414)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CEFR", std::make_unique<machine_instruction>("CEFR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1415)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDFR", std::make_unique<machine_instruction>("CDFR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1415)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CXFR", std::make_unique<machine_instruction>("CXFR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1415)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CEGR", std::make_unique<machine_instruction>("CEGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1415)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDGR", std::make_unique<machine_instruction>("CDGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1415)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CXGR", std::make_unique<machine_instruction>("CXGR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1415)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CFER", std::make_unique<machine_instruction>("CFER", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U }, 32, 1415)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CFDR", std::make_unique<machine_instruction>("CFDR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U }, 32, 1415)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CFXR", std::make_unique<machine_instruction>("CFXR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U }, 32, 1415)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGER", std::make_unique<machine_instruction>("CGER", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U }, 32, 1415)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGDR", std::make_unique<machine_instruction>("CGDR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U }, 32, 1415)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGXR", std::make_unique<machine_instruction>("CGXR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U }, 32, 1415)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DDR", std::make_unique<machine_instruction>("DDR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1416)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DER", std::make_unique<machine_instruction>("DER", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1416)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DXR", std::make_unique<machine_instruction>("DXR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1416)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DD", std::make_unique<machine_instruction>("DD", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 1416)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DE", std::make_unique<machine_instruction>("DE", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 1416)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("HDR", std::make_unique<machine_instruction>("HDR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1417)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("HER", std::make_unique<machine_instruction>("HER", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1417)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LTER", std::make_unique<machine_instruction>("LTER", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1417)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LTDR", std::make_unique<machine_instruction>("LTDR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1417)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LTXR", std::make_unique<machine_instruction>("LTXR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1418)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LCER", std::make_unique<machine_instruction>("LCER", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1418)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LCDR", std::make_unique<machine_instruction>("LCDR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1418)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LCXR", std::make_unique<machine_instruction>("LCXR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1418)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("FIER", std::make_unique<machine_instruction>("FIER", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1419)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("FIDR", std::make_unique<machine_instruction>("FIDR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1419)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("FIXR", std::make_unique<machine_instruction>("FIXR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1419)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LDER", std::make_unique<machine_instruction>("LDER", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1419)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LXDR", std::make_unique<machine_instruction>("LXDR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1419)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LXER", std::make_unique<machine_instruction>("LXER", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1419)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LDE", std::make_unique<machine_instruction>("LDE", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U}, 48, 1419)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LXD", std::make_unique<machine_instruction>("LXD", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U}, 48, 1419)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LXE", std::make_unique<machine_instruction>("LXE", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U}, 48, 1419)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LNDR", std::make_unique<machine_instruction>("LNDR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1420)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LNER", std::make_unique<machine_instruction>("LNER", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1420)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LPDR", std::make_unique<machine_instruction>("LPDR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1420)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LPER", std::make_unique<machine_instruction>("LPER", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1420)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LNXR", std::make_unique<machine_instruction>("LNXR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1420)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LPXR", std::make_unique<machine_instruction>("LPXR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1420)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LEDR", std::make_unique<machine_instruction>("LEDR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1421)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LDXR", std::make_unique<machine_instruction>("LDXR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1421)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LRER", std::make_unique<machine_instruction>("LRER", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1421)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LRDR", std::make_unique<machine_instruction>("LRDR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1421)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LEXR", std::make_unique<machine_instruction>("LEXR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1421)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MEER", std::make_unique<machine_instruction>("MEER", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1421)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MDR", std::make_unique<machine_instruction>("MDR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1421)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MXR", std::make_unique<machine_instruction>("MXR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1421)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MDER", std::make_unique<machine_instruction>("MDER", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1421)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MER", std::make_unique<machine_instruction>("MER", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1421)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MXDR", std::make_unique<machine_instruction>("MXDR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1421)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MEE", std::make_unique<machine_instruction>("MEE", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1422)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MD", std::make_unique<machine_instruction>("MD", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 1422)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MDE", std::make_unique<machine_instruction>("MDE", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 1422)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MXD", std::make_unique<machine_instruction>("MXD", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 1422)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ME", std::make_unique<machine_instruction>("ME", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 1422)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MAER", std::make_unique<machine_instruction>("MAER", mach_format::RRD, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1423)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MADR", std::make_unique<machine_instruction>("MADR", mach_format::RRD, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1423)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MAD", std::make_unique<machine_instruction>("MAD", mach_format::RXF, std::vector<machine_operand_format>{reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1423)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MAE", std::make_unique<machine_instruction>("MAE", mach_format::RXF, std::vector<machine_operand_format>{reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1423)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSER", std::make_unique<machine_instruction>("MSER", mach_format::RRD, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1423)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSDR", std::make_unique<machine_instruction>("MSDR", mach_format::RRD, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1423)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSE", std::make_unique<machine_instruction>("MSE", mach_format::RXF, std::vector<machine_operand_format>{reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1423)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSD", std::make_unique<machine_instruction>("MSD", mach_format::RXF, std::vector<machine_operand_format>{reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1423)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MAYR", std::make_unique<machine_instruction>("MAYR", mach_format::RRD, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1424)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MAYHR", std::make_unique<machine_instruction>("MAYHR", mach_format::RRD, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1424)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MAYLR", std::make_unique<machine_instruction>("MAYLR", mach_format::RRD, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1424)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MAY", std::make_unique<machine_instruction>("MAY", mach_format::RXF, std::vector<machine_operand_format>{reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1424)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MAYH", std::make_unique<machine_instruction>("MAYH", mach_format::RXF, std::vector<machine_operand_format>{reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1424)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MAYL", std::make_unique<machine_instruction>("MAYL", mach_format::RXF, std::vector<machine_operand_format>{reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1424)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MYR", std::make_unique<machine_instruction>("MYR", mach_format::RRD, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1426)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MYHR", std::make_unique<machine_instruction>("MYHR", mach_format::RRD, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1426)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MYLR", std::make_unique<machine_instruction>("MYLR", mach_format::RRD, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1426)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MY", std::make_unique<machine_instruction>("MY", mach_format::RXF, std::vector<machine_operand_format>{reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1426)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MYH", std::make_unique<machine_instruction>("MYH", mach_format::RXF, std::vector<machine_operand_format>{reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1426)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MYL", std::make_unique<machine_instruction>("MYL", mach_format::RXF, std::vector<machine_operand_format>{reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1426)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SQER", std::make_unique<machine_instruction>("SQER", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1427)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SQDR", std::make_unique<machine_instruction>("SQDR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1427)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SQXR", std::make_unique<machine_instruction>("SQXR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1427)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SQE", std::make_unique<machine_instruction>("SQE", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1427)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SQD", std::make_unique<machine_instruction>("SQD", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1427)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SER", std::make_unique<machine_instruction>("SER", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1428)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SDR", std::make_unique<machine_instruction>("SDR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1428)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SXR", std::make_unique<machine_instruction>("SXR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1428)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SE", std::make_unique<machine_instruction>("SE", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 1428)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SD", std::make_unique<machine_instruction>("SD", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 1428)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SUR", std::make_unique<machine_instruction>("SUR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1429)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SWR", std::make_unique<machine_instruction>("SWR", mach_format::RR, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 16, 1429)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SU", std::make_unique<machine_instruction>("SU", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 1429)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SW", std::make_unique<machine_instruction>("SW", mach_format::RX_a, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 32, 1429)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AEBR", std::make_unique<machine_instruction>("AEBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1445)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ADBR", std::make_unique<machine_instruction>("ADBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1445)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AXBR", std::make_unique<machine_instruction>("AXBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1445)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AEB", std::make_unique<machine_instruction>("AEB", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1445)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ADB", std::make_unique<machine_instruction>("ADB", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1445)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CEBR", std::make_unique<machine_instruction>("CEBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1447)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDBR", std::make_unique<machine_instruction>("CDBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1447)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CXBR", std::make_unique<machine_instruction>("CXBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1447)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDB", std::make_unique<machine_instruction>("CDB", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1447)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CEB", std::make_unique<machine_instruction>("CEB", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1447)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("KEBR", std::make_unique<machine_instruction>("KEBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1448)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("KDBR", std::make_unique<machine_instruction>("KDBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1448)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("KXBR", std::make_unique<machine_instruction>("KXBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1448)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("KDB", std::make_unique<machine_instruction>("KDB", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1448)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("KEB", std::make_unique<machine_instruction>("KEB", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1448)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CEFBR", std::make_unique<machine_instruction>("CEFBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1449)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDFBR", std::make_unique<machine_instruction>("CDFBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1449)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CXFBR", std::make_unique<machine_instruction>("CXFBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1449)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CEGBR", std::make_unique<machine_instruction>("CEGBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1449)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDGBR", std::make_unique<machine_instruction>("CDGBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1449)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CXGBR", std::make_unique<machine_instruction>("CXGBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1449)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CEFBRA", std::make_unique<machine_instruction>("CEFBRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1449)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDFBRA", std::make_unique<machine_instruction>("CDFBRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1449)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CXFBRA", std::make_unique<machine_instruction>("CXFBRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1449)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CEGBRA", std::make_unique<machine_instruction>("CEGBRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1449)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDGBRA", std::make_unique<machine_instruction>("CDGBRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1449)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CXGBRA", std::make_unique<machine_instruction>("CXGBRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1449)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CELFBR", std::make_unique<machine_instruction>("CELFBR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1451)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDLFBR", std::make_unique<machine_instruction>("CDLFBR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1451)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CXLFBR", std::make_unique<machine_instruction>("CXLFBR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1451)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CELGBR", std::make_unique<machine_instruction>("CELGBR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1451)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDLGBR", std::make_unique<machine_instruction>("CDLGBR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1451)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CXLGBR", std::make_unique<machine_instruction>("CXLGBR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1451)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CFEBR", std::make_unique<machine_instruction>("CFEBR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U }, 32, 1452)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CFDBR", std::make_unique<machine_instruction>("CFDBR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U }, 32, 1452)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CFXBR", std::make_unique<machine_instruction>("CFXBR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U }, 32, 1452)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGEBR", std::make_unique<machine_instruction>("CGEBR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U }, 32, 1452)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGDBR", std::make_unique<machine_instruction>("CGDBR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U }, 32, 1452)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGXBR", std::make_unique<machine_instruction>("CGXBR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U }, 32, 1452)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CFEBRA", std::make_unique<machine_instruction>("CFEBRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1452)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CFDBRA", std::make_unique<machine_instruction>("CFDBRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1452)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CFXBRA", std::make_unique<machine_instruction>("CFXBRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1452)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGEBRA", std::make_unique<machine_instruction>("CGEBRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1452)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGDBRA", std::make_unique<machine_instruction>("CGDBRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1452)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGXBRA", std::make_unique<machine_instruction>("CGXBRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1452)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLFEBR", std::make_unique<machine_instruction>("CLFEBR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1455)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLFDBR", std::make_unique<machine_instruction>("CLFDBR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1455)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLFXBR", std::make_unique<machine_instruction>("CLFXBR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1455)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLGEBR", std::make_unique<machine_instruction>("CLGEBR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1455)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLGDBR", std::make_unique<machine_instruction>("CLGDBR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1455)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLGXBR", std::make_unique<machine_instruction>("CLGXBR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1455)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DEBR", std::make_unique<machine_instruction>("DEBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1457)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DDBR", std::make_unique<machine_instruction>("DDBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1457)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DXBR", std::make_unique<machine_instruction>("DXBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1457)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DEB", std::make_unique<machine_instruction>("DEB", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1457)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DDB", std::make_unique<machine_instruction>("DDB", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1457)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DIEBR", std::make_unique<machine_instruction>("DIEBR", mach_format::RRF_b, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1458)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DIDBR", std::make_unique<machine_instruction>("DIDBR", mach_format::RRF_b, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1458)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LTEBR", std::make_unique<machine_instruction>("LTEBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1461)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LTDBR", std::make_unique<machine_instruction>("LTDBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1461)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LTXBR", std::make_unique<machine_instruction>("LTXBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1461)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LCEBR", std::make_unique<machine_instruction>("LCEBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1461)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LCDBR", std::make_unique<machine_instruction>("LCDBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1461)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LCXBR", std::make_unique<machine_instruction>("LCXBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1461)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("FIEBR", std::make_unique<machine_instruction>("FIEBR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U }, 32, 1462)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("FIDBR", std::make_unique<machine_instruction>("FIDBR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U }, 32, 1462)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("FIXBR", std::make_unique<machine_instruction>("FIXBR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U }, 32, 1462)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("FIEBRA", std::make_unique<machine_instruction>("FIEBRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1462)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("FIDBRA", std::make_unique<machine_instruction>("FIDBRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1462)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("FIXBRA", std::make_unique<machine_instruction>("FIXBRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1462)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LDEBR", std::make_unique<machine_instruction>("LDEBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1463)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LXDBR", std::make_unique<machine_instruction>("LXDBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1463)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LXEBR", std::make_unique<machine_instruction>("LXEBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1463)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LDEB", std::make_unique<machine_instruction>("LDEB", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 48, 1464)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LXDB", std::make_unique<machine_instruction>("LXDB", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 48, 1464)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LXEB", std::make_unique<machine_instruction>("LXEB", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 48, 1464)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LNEBR", std::make_unique<machine_instruction>("LNEBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1464)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LNDBR", std::make_unique<machine_instruction>("LNDBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1464)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LNXBR", std::make_unique<machine_instruction>("LNXBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1464)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LPEBR", std::make_unique<machine_instruction>("LPEBR", mach_format::RRE, std::vector<machine_operand_format>{ reg_4_U, reg_4_U}, 32, 1465)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LPDBR", std::make_unique<machine_instruction>("LPDBR", mach_format::RRE, std::vector<machine_operand_format>{ reg_4_U, reg_4_U}, 32, 1465)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LPXBR", std::make_unique<machine_instruction>("LPXBR", mach_format::RRE, std::vector<machine_operand_format>{ reg_4_U, reg_4_U}, 32, 1465)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LEDBR", std::make_unique<machine_instruction>("LEDBR", mach_format::RRE, std::vector<machine_operand_format>{ reg_4_U, reg_4_U}, 32, 1465)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LDXBR", std::make_unique<machine_instruction>("LDXBR", mach_format::RRE, std::vector<machine_operand_format>{ reg_4_U, reg_4_U}, 32, 1465)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LEXBR", std::make_unique<machine_instruction>("LEXBR", mach_format::RRE, std::vector<machine_operand_format>{ reg_4_U, reg_4_U}, 32, 1465)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LEDBRA", std::make_unique<machine_instruction>("LEDBRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1465)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LDXBRA", std::make_unique<machine_instruction>("LDXBRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1465)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LEXBRA", std::make_unique<machine_instruction>("LEXBRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1465)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MEEBR", std::make_unique<machine_instruction>("MEEBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1467)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MDBR", std::make_unique<machine_instruction>("MDBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1467)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MXBR", std::make_unique<machine_instruction>("MXBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1467)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MDEBR", std::make_unique<machine_instruction>("MDEBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1467)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MXDBR", std::make_unique<machine_instruction>("MXDBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1467)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MEEB", std::make_unique<machine_instruction>("MEEB", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1467)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MDB", std::make_unique<machine_instruction>("MDB", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1467)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MDEB", std::make_unique<machine_instruction>("MDEB", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1467)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MXDB", std::make_unique<machine_instruction>("MXDB", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1467)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MADBR", std::make_unique<machine_instruction>("MADBR", mach_format::RRD, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1468)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MAEBR", std::make_unique<machine_instruction>("MAEBR", mach_format::RRD, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1468)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MAEB", std::make_unique<machine_instruction>("MAEB", mach_format::RXF, std::vector<machine_operand_format>{reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1468)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MADB", std::make_unique<machine_instruction>("MADB", mach_format::RXF, std::vector<machine_operand_format>{reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1468)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSEBR", std::make_unique<machine_instruction>("MSEBR", mach_format::RRD, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1468)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSDBR", std::make_unique<machine_instruction>("MSDBR", mach_format::RRD, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1468)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSEB", std::make_unique<machine_instruction>("MSEB", mach_format::RXF, std::vector<machine_operand_format>{reg_4_U, reg_4_U, dxb_12_4x4_U}, 48, 1468)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MSDB", std::make_unique<machine_instruction>("MSDB", mach_format::RXF, std::vector<machine_operand_format>{reg_4_U, reg_4_U, dxb_12_4x4_U}, 48, 1468)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SQEBR", std::make_unique<machine_instruction>("SQEBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1470)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SQDBR", std::make_unique<machine_instruction>("SQDBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1470)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SQXBR", std::make_unique<machine_instruction>("SQXBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1470)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SQEB", std::make_unique<machine_instruction>("SQEB", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1470)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SQDB", std::make_unique<machine_instruction>("SQDB", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1470)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SEBR", std::make_unique<machine_instruction>("SEBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1470)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SDBR", std::make_unique<machine_instruction>("SDBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1470)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SXBR", std::make_unique<machine_instruction>("SXBR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1470)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SEB", std::make_unique<machine_instruction>("SEB", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U}, 48, 1470)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SDB", std::make_unique<machine_instruction>("SDB", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U}, 48, 1470)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TCEB", std::make_unique<machine_instruction>("TCEB", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1471)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TCDB", std::make_unique<machine_instruction>("TCDB", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1471)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TCXB", std::make_unique<machine_instruction>("TCXB", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1471)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ADTR", std::make_unique<machine_instruction>("ADTR", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1491)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AXTR", std::make_unique<machine_instruction>("AXTR", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1491)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ADTRA", std::make_unique<machine_instruction>("ADTRA", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1491)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("AXTRA", std::make_unique<machine_instruction>("AXTRA", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1491)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDTR", std::make_unique<machine_instruction>("CDTR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1494)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CXTR", std::make_unique<machine_instruction>("CXTR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1494)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("KDTR", std::make_unique<machine_instruction>("KDTR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1495)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("KXTR", std::make_unique<machine_instruction>("KXTR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1495)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CEDTR", std::make_unique<machine_instruction>("CEDTR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1495)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CEXTR", std::make_unique<machine_instruction>("CEXTR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1495)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDGTR", std::make_unique<machine_instruction>("CDGTR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1496)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CXGTR", std::make_unique<machine_instruction>("CXGTR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1496)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDGTRA", std::make_unique<machine_instruction>("CDGTRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1496)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CXGTRA", std::make_unique<machine_instruction>("CXGTRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1496)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDFTR", std::make_unique<machine_instruction>("CDFTR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1496)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CXFTR", std::make_unique<machine_instruction>("CXFTR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1496)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDLGTR", std::make_unique<machine_instruction>("CDLGTR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1497)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CXLGTR", std::make_unique<machine_instruction>("CXLGTR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1497)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDLFTR", std::make_unique<machine_instruction>("CDLFTR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1497)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CXLFTR", std::make_unique<machine_instruction>("CXLFTR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1497)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDPT", std::make_unique<machine_instruction>("CDPT", mach_format::RSL_b, std::vector<machine_operand_format>{reg_4_U, db_12_8x4L_U, mask_4_U }, 48, 1498)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CXPT", std::make_unique<machine_instruction>("CXPT", mach_format::RSL_b, std::vector<machine_operand_format>{reg_4_U, db_12_8x4L_U, mask_4_U }, 48, 1498)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDSTR", std::make_unique<machine_instruction>("CDSTR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1500)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CXSTR", std::make_unique<machine_instruction>("CXSTR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1500)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDUTR", std::make_unique<machine_instruction>("CDUTR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1500)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CXUTR", std::make_unique<machine_instruction>("CXUTR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1500)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CDZT", std::make_unique<machine_instruction>("CDZT", mach_format::RSL_b, std::vector<machine_operand_format>{ reg_4_U, db_12_8x4L_U, mask_4_U }, 48, 1501)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CXZT", std::make_unique<machine_instruction>("CXZT", mach_format::RSL_b, std::vector<machine_operand_format>{reg_4_U, db_12_8x4L_U, mask_4_U }, 48, 1501)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGDTR", std::make_unique<machine_instruction>("CGDTR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U }, 32, 1501)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGXTR", std::make_unique<machine_instruction>("CGXTR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U }, 32, 1501)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGDTRA", std::make_unique<machine_instruction>("CGDTRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1502)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CGXTRA", std::make_unique<machine_instruction>("CGXTRA", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1502)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CFDTR", std::make_unique<machine_instruction>("CFDTR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1502)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CFXTR", std::make_unique<machine_instruction>("CFXTR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1502)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLGDTR", std::make_unique<machine_instruction>("CLGDTR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1504)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLGXTR", std::make_unique<machine_instruction>("CLGXTR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1504)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLFDTR", std::make_unique<machine_instruction>("CLFDTR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1504)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CLFXTR", std::make_unique<machine_instruction>("CLFXTR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1504)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CPDT", std::make_unique<machine_instruction>("CPDT", mach_format::RSL_b, std::vector<machine_operand_format>{reg_4_U, db_12_8x4L_U, mask_4_U }, 48, 1505)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CPXT", std::make_unique<machine_instruction>("CPXT", mach_format::RSL_b, std::vector<machine_operand_format>{reg_4_U, db_12_8x4L_U, mask_4_U }, 48, 1505)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CSDTR", std::make_unique<machine_instruction>("CSDTR", mach_format::RRF_d, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 32, 1507)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CSXTR", std::make_unique<machine_instruction>("CSXTR", mach_format::RRF_d, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 32, 1507)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CUDTR", std::make_unique<machine_instruction>("CUDTR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1507)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CUXTR", std::make_unique<machine_instruction>("CUXTR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1507)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CZDT", std::make_unique<machine_instruction>("CZDT", mach_format::RSL_b, std::vector<machine_operand_format>{reg_4_U, db_12_8x4L_U, mask_4_U }, 48, 1508)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("CZXT", std::make_unique<machine_instruction>("CZXT", mach_format::RSL_b, std::vector<machine_operand_format>{reg_4_U, db_12_8x4L_U, mask_4_U }, 48, 1508)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DDTR", std::make_unique<machine_instruction>("DDTR", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1509)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DXTR", std::make_unique<machine_instruction>("DXTR", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1509)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DDTRA", std::make_unique<machine_instruction>("DDTRA", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1509)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("DXTRA", std::make_unique<machine_instruction>("DXTRA", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1509)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("EEXTR", std::make_unique<machine_instruction>("EEXTR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1511)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("EEDTR", std::make_unique<machine_instruction>("EEDTR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1511)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ESDTR", std::make_unique<machine_instruction>("ESDTR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1511)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("ESXTR", std::make_unique<machine_instruction>("ESXTR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1511)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("IEDTR", std::make_unique<machine_instruction>("IEDTR", mach_format::RRF_b, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1512)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("IEXTR", std::make_unique<machine_instruction>("IEXTR", mach_format::RRF_b, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1512)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LTDTR", std::make_unique<machine_instruction>("LTDTR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1513)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LTXTR", std::make_unique<machine_instruction>("LTXTR", mach_format::RRE, std::vector<machine_operand_format>{reg_4_U, reg_4_U }, 32, 1513)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("FIDTR", std::make_unique<machine_instruction>("FIDTR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1514)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("FIXTR", std::make_unique<machine_instruction>("FIXTR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1514)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LDETR", std::make_unique<machine_instruction>("LDETR", mach_format::RRF_d, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 32, 1517)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LXDTR", std::make_unique<machine_instruction>("LXDTR", mach_format::RRF_d, std::vector<machine_operand_format>{reg_4_U, reg_4_U, mask_4_U }, 32, 1517)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LEDTR", std::make_unique<machine_instruction>("LEDTR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1518)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("LDXTR", std::make_unique<machine_instruction>("LDXTR", mach_format::RRF_e, std::vector<machine_operand_format>{reg_4_U, mask_4_U, reg_4_U, mask_4_U }, 32, 1518)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MDTR", std::make_unique<machine_instruction>("MDTR", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1519)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MXTR", std::make_unique<machine_instruction>("MXTR", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1519)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MDTRA", std::make_unique<machine_instruction>("MDTRA", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1520)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("MXTRA", std::make_unique<machine_instruction>("MXTRA", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1520)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("QADTR", std::make_unique<machine_instruction>("QADTR", mach_format::RRF_b, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1521)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("QAXTR", std::make_unique<machine_instruction>("QAXTR", mach_format::RRF_b, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1521)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("RRDTR", std::make_unique<machine_instruction>("RRDTR", mach_format::RRF_b, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1524)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("RRXTR", std::make_unique<machine_instruction>("RRXTR", mach_format::RRF_b, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1524)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLDT", std::make_unique<machine_instruction>("SLDT", mach_format::RXF, std::vector<machine_operand_format>{reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1526)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SLXT", std::make_unique<machine_instruction>("SLXT", mach_format::RXF, std::vector<machine_operand_format>{reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1526)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SRDT", std::make_unique<machine_instruction>("SRDT", mach_format::RXF, std::vector<machine_operand_format>{reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1526)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SRXT", std::make_unique<machine_instruction>("SRXT", mach_format::RXF, std::vector<machine_operand_format>{reg_4_U, reg_4_U, dxb_12_4x4_U }, 48, 1526)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SDTR", std::make_unique<machine_instruction>("SDTR", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1527)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SXTR", std::make_unique<machine_instruction>("SXTR", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U }, 32, 1527)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SDTRA", std::make_unique<machine_instruction>("SDTRA", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1527)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("SXTRA", std::make_unique<machine_instruction>("SXTRA", mach_format::RRF_a, std::vector<machine_operand_format>{reg_4_U, reg_4_U, reg_4_U, mask_4_U }, 32, 1527)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TDCET", std::make_unique<machine_instruction>("TDCET", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1528)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TDCDT", std::make_unique<machine_instruction>("TDCDT", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1528)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TDCXT", std::make_unique<machine_instruction>("TDCXT", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1528)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TDGET", std::make_unique<machine_instruction>("TDGET", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1529)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TDGDT", std::make_unique<machine_instruction>("TDGDT", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1529)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("TDGXT", std::make_unique<machine_instruction>("TDGXT", mach_format::RXE, std::vector<machine_operand_format>{reg_4_U, dxb_12_4x4_U }, 48, 1529)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VBPERM", std::make_unique<machine_instruction>("VBPERM", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1536)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VGEF", std::make_unique<machine_instruction>("VGEF", mach_format::VRV, std::vector<machine_operand_format>{vec_reg_4_U, dvb_12_4x4_U, mask_4_U }, 48, 1536)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VGEG", std::make_unique<machine_instruction>("VGEG", mach_format::VRV, std::vector<machine_operand_format>{vec_reg_4_U, dvb_12_4x4_U, mask_4_U }, 48, 1536)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VGBM", std::make_unique<machine_instruction>("VGBM", mach_format::VRI_a, std::vector<machine_operand_format>{vec_reg_4_U, imm_16_U }, 48, 1537)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VGM", std::make_unique<machine_instruction>("VGM", mach_format::VRI_b, std::vector<machine_operand_format>{vec_reg_4_U, imm_8_U, imm_8_U, mask_4_U }, 48, 1537)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VL", std::make_unique<machine_instruction>("VL", mach_format::VRX, std::vector<machine_operand_format>{ vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 1, 48, 1538)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLREP", std::make_unique<machine_instruction>("VLREP", mach_format::VRX, std::vector<machine_operand_format>{vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1538)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLR", std::make_unique<machine_instruction>("VLR", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U }, 48, 1538)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLEB", std::make_unique<machine_instruction>("VLEB", mach_format::VRX, std::vector<machine_operand_format>{vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1538)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLEH", std::make_unique<machine_instruction>("VLEH", mach_format::VRX, std::vector<machine_operand_format>{vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1539)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLEIH", std::make_unique<machine_instruction>("VLEIH", mach_format::VRI_a, std::vector<machine_operand_format>{vec_reg_4_U, imm_16_S , mask_4_U }, 48, 1539)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLEF", std::make_unique<machine_instruction>("VLEF", mach_format::VRX, std::vector<machine_operand_format>{vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1539)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLEIF", std::make_unique<machine_instruction>("VLEIF", mach_format::VRI_a, std::vector<machine_operand_format>{vec_reg_4_U, imm_16_S , mask_4_U }, 48, 1539)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLEG", std::make_unique<machine_instruction>("VLEG", mach_format::VRX, std::vector<machine_operand_format>{vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1539)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLEIG", std::make_unique<machine_instruction>("VLEIG", mach_format::VRI_a, std::vector<machine_operand_format>{vec_reg_4_U, imm_16_S , mask_4_U }, 48, 1539)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLEIB", std::make_unique<machine_instruction>("VLEIB", mach_format::VRI_a, std::vector<machine_operand_format>{vec_reg_4_U, imm_16_S , mask_4_U }, 48, 1539)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLGV", std::make_unique<machine_instruction>("VLGV", mach_format::VRS_c, std::vector<machine_operand_format>{reg_4_U, vec_reg_4_U, db_12_4_U, mask_4_U }, 48, 1539)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLLEZ", std::make_unique<machine_instruction>("VLLEZ", mach_format::VRX, std::vector<machine_operand_format>{vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1540)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLM", std::make_unique<machine_instruction>("VLM", mach_format::VRS_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, db_12_4_U, mask_4_U }, 1, 48, 1541)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLRLR", std::make_unique<machine_instruction>("VLRLR", mach_format::VRS_d, std::vector<machine_operand_format>{vec_reg_4_U, reg_4_U, db_12_4_U }, 48, 1541)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLRL", std::make_unique<machine_instruction>("VLRL", mach_format::VSI, std::vector<machine_operand_format>{vec_reg_4_U, db_12_4_U, imm_8_U }, 48, 1541)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLBB", std::make_unique<machine_instruction>("VLBB", mach_format::VRX, std::vector<machine_operand_format>{vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1542)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLVG", std::make_unique<machine_instruction>("VLVG", mach_format::VRS_b, std::vector<machine_operand_format>{vec_reg_4_U, reg_4_U, db_12_4_U, mask_4_U }, 48, 1543)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLVGP", std::make_unique<machine_instruction>("VLVGP", mach_format::VRR_f, std::vector<machine_operand_format>{vec_reg_4_U, reg_4_U, reg_4_U }, 48, 1543)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLL", std::make_unique<machine_instruction>("VLL", mach_format::VRS_b, std::vector<machine_operand_format>{vec_reg_4_U, reg_4_U, db_12_4_U }, 48, 1543)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VMRH", std::make_unique<machine_instruction>("VMRH", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1544)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VMRL", std::make_unique<machine_instruction>("VMRL", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1544)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VPK", std::make_unique<machine_instruction>("VPK", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1545)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VPKS", std::make_unique<machine_instruction>("VPKS", mach_format::VRR_b, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1545)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VPKLS", std::make_unique<machine_instruction>("VPKLS", mach_format::VRR_b, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1546)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VPERM", std::make_unique<machine_instruction>("VPERM", mach_format::VRR_e, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1547)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VPDI", std::make_unique<machine_instruction>("VPDI", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1547)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VREP", std::make_unique<machine_instruction>("VREP", mach_format::VRI_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, imm_16_U, mask_4_U }, 48, 1547)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VREPI", std::make_unique<machine_instruction>("VREPI", mach_format::VRI_a, std::vector<machine_operand_format>{vec_reg_4_U, imm_16_S , mask_4_U }, 48, 1548)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSCEF", std::make_unique<machine_instruction>("VSCEF", mach_format::VRV, std::vector<machine_operand_format>{vec_reg_4_U, dvb_12_4x4_U, mask_4_U }, 48, 1548)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSCEG", std::make_unique<machine_instruction>("VSCEG", mach_format::VRV, std::vector<machine_operand_format>{vec_reg_4_U, dvb_12_4x4_U, mask_4_U }, 48, 1548)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSEL", std::make_unique<machine_instruction>("VSEL", mach_format::VRR_e, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1549)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSEG", std::make_unique<machine_instruction>("VSEG", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1549)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VST", std::make_unique<machine_instruction>("VST", mach_format::VRX, std::vector<machine_operand_format>{vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 1, 48, 1550)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSTEB", std::make_unique<machine_instruction>("VSTEB", mach_format::VRX, std::vector<machine_operand_format>{vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1550)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSTEH", std::make_unique<machine_instruction>("VSTEH", mach_format::VRX, std::vector<machine_operand_format>{vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1550)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSTEF", std::make_unique<machine_instruction>("VSTEF", mach_format::VRX, std::vector<machine_operand_format>{vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1550)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSTEG", std::make_unique<machine_instruction>("VSTEG", mach_format::VRX, std::vector<machine_operand_format>{vec_reg_4_U, dxb_12_4x4_U, mask_4_U }, 48, 1550)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSTM", std::make_unique<machine_instruction>("VSTM", mach_format::VRS_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, db_12_4_U, mask_4_U }, 1, 48, 1551)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSTRLR", std::make_unique<machine_instruction>("VSTRLR", mach_format::VRS_d, std::vector<machine_operand_format>{vec_reg_4_U, reg_4_U, db_12_4_U }, 48, 1551)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSTRL", std::make_unique<machine_instruction>("VSTRL", mach_format::VSI, std::vector<machine_operand_format>{vec_reg_4_U, db_12_4_U, imm_8_U }, 48, 1551)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSTL", std::make_unique<machine_instruction>("VSTL", mach_format::VRS_b, std::vector<machine_operand_format>{vec_reg_4_U, reg_4_U, db_12_4_U }, 48, 1552)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VUPH", std::make_unique<machine_instruction>("VUPH", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1552)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VUPL", std::make_unique<machine_instruction>("VUPL", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1553)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VUPLH", std::make_unique<machine_instruction>("VUPLH", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1553)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VUPLL", std::make_unique<machine_instruction>("VUPLL", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1554)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VA", std::make_unique<machine_instruction>("VA", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1557)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VACC", std::make_unique<machine_instruction>("VACC", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1558)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VAC", std::make_unique<machine_instruction>("VAC", mach_format::VRR_d, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1558)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VACCC", std::make_unique<machine_instruction>("VACCC", mach_format::VRR_d, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1559)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VN", std::make_unique<machine_instruction>("VN", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1559)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VNC", std::make_unique<machine_instruction>("VNC", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1559)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VAVG", std::make_unique<machine_instruction>("VAVG", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1560)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VAVGL", std::make_unique<machine_instruction>("VAVGL", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1560)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VCKSM", std::make_unique<machine_instruction>("VCKSM", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1560)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VEC", std::make_unique<machine_instruction>("VEC", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1561)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VECL", std::make_unique<machine_instruction>("VECL", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1561)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VCEQ", std::make_unique<machine_instruction>("VCEQ", mach_format::VRR_b, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1561)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VCH", std::make_unique<machine_instruction>("VCH", mach_format::VRR_b, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1562)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VCHL", std::make_unique<machine_instruction>("VCHL", mach_format::VRR_b, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1563)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VCLZ", std::make_unique<machine_instruction>("VCLZ", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1564)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VCTZ", std::make_unique<machine_instruction>("VCTZ", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1564)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VGFM", std::make_unique<machine_instruction>("VGFM", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1565)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VX", std::make_unique<machine_instruction>("VX", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1565)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLC", std::make_unique<machine_instruction>("VLC", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1566)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VGFMA", std::make_unique<machine_instruction>("VGFMA", mach_format::VRR_d, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1566)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLP", std::make_unique<machine_instruction>("VLP", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1566)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VMX", std::make_unique<machine_instruction>("VMX", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1567)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VMXL", std::make_unique<machine_instruction>("VMXL", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1567)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VMN", std::make_unique<machine_instruction>("VMN", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1567)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VMNL", std::make_unique<machine_instruction>("VMNL", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1568)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VMAL", std::make_unique<machine_instruction>("VMAL", mach_format::VRR_d, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1568)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VMAH", std::make_unique<machine_instruction>("VMAH", mach_format::VRR_d, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1569)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VMALH", std::make_unique<machine_instruction>("VMALH", mach_format::VRR_d, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1569)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VMAE", std::make_unique<machine_instruction>("VMAE", mach_format::VRR_d, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1569)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VMALE", std::make_unique<machine_instruction>("VMALE", mach_format::VRR_d, std::vector<machine_operand_format>{ vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1569)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VMAO", std::make_unique<machine_instruction>("VMAO", mach_format::VRR_d, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1570)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VMALO", std::make_unique<machine_instruction>("VMALO", mach_format::VRR_d, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1570)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VMH", std::make_unique<machine_instruction>("VMH", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1570)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VMLH", std::make_unique<machine_instruction>("VMLH", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1571)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VML", std::make_unique<machine_instruction>("VML", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1571)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VME", std::make_unique<machine_instruction>("VME", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1572)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VMLE", std::make_unique<machine_instruction>("VMLE", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1572)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VMO", std::make_unique<machine_instruction>("VMO", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1572)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VMLO", std::make_unique<machine_instruction>("VMLO", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1572)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VMSL", std::make_unique<machine_instruction>("VMSL", mach_format::VRR_d, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1573)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VNN", std::make_unique<machine_instruction>("VNN", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1574)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VNO", std::make_unique<machine_instruction>("VNO", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1574)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VNOT", std::make_unique<vnot_instruction>("VNOT", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1574)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VNX", std::make_unique<machine_instruction>("VNX", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1574)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VO", std::make_unique<machine_instruction>("VO", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1574)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VOC", std::make_unique<machine_instruction>("VOC", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1575)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VPOPCT", std::make_unique<machine_instruction>("VPOPCT", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1575)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VERLLV", std::make_unique<machine_instruction>("VERLLV", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1575)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VERLL", std::make_unique<machine_instruction>("VERLL", mach_format::VRS_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, db_12_4_U, mask_4_U }, 48, 1575)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VERIM", std::make_unique<machine_instruction>("VERIM", mach_format::VRI_d, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, imm_8_U, mask_4_U }, 48, 1576)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VESLV", std::make_unique<machine_instruction>("VESLV", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1577)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VESL", std::make_unique<machine_instruction>("VESL", mach_format::VRS_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, db_12_4_U, mask_4_U }, 48, 1577)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VESRAV", std::make_unique<machine_instruction>("VESRAV", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1577)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VESRA", std::make_unique<machine_instruction>("VESRA", mach_format::VRS_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, db_12_4_U, mask_4_U }, 48, 1577)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VESRLV", std::make_unique<machine_instruction>("VESRLV", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1578)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VESRL", std::make_unique<machine_instruction>("VESRL", mach_format::VRS_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, db_12_4_U, mask_4_U }, 48, 1578)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSLDB", std::make_unique<machine_instruction>("VSLDB", mach_format::VRI_d, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, imm_8_U }, 48, 1579)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSL", std::make_unique<machine_instruction>("VSL", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1579)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSLB", std::make_unique<machine_instruction>("VSLB", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1579)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSRA", std::make_unique<machine_instruction>("VSRA", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1579)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSRAB", std::make_unique<machine_instruction>("VSRAB", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1580)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSRL", std::make_unique<machine_instruction>("VSRL", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1580)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSRLB", std::make_unique<machine_instruction>("VSRLB", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U }, 48, 1580)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VS", std::make_unique<machine_instruction>("VS", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1580)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSCBI", std::make_unique<machine_instruction>("VSCBI", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1581)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSBI", std::make_unique<machine_instruction>("VSBI", mach_format::VRR_d, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1581)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSBCBI", std::make_unique<machine_instruction>("VSBCBI", mach_format::VRR_d, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1582)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSUMG", std::make_unique<machine_instruction>("VSUMG", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1582)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSUMQ", std::make_unique<machine_instruction>("VSUMQ", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1583)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSUM", std::make_unique<machine_instruction>("VSUM", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1583)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VTM", std::make_unique<machine_instruction>("VTM", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U }, 48, 1584)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFAE", std::make_unique<machine_instruction>("VFAE", mach_format::VRR_b, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 1, 48, 1585)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFEE", std::make_unique<machine_instruction>("VFEE", mach_format::VRR_b, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 1, 48, 1587)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFENE", std::make_unique<machine_instruction>("VFENE", mach_format::VRR_b, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 1, 48, 1588)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VISTR", std::make_unique<machine_instruction>("VISTR", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 1, 48, 1589)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSTRC", std::make_unique<machine_instruction>("VSTRC", mach_format::VRR_d, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 1, 48, 1590)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFA", std::make_unique<machine_instruction>("VFA", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1595)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("WFC", std::make_unique<machine_instruction>("WFC", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1599)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("WFK", std::make_unique<machine_instruction>("WFK", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1600)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFCE", std::make_unique<machine_instruction>("VFCE", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U }, 48, 1601)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFCH", std::make_unique<machine_instruction>("VFCH", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U }, 48, 1603)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFCHE", std::make_unique<machine_instruction>("VFCHE", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U }, 48, 1605)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VCDG", std::make_unique<machine_instruction>("VCDG", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U }, 48, 1607)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VCDLG", std::make_unique<machine_instruction>("VCDLG", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U }, 48, 1608)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VCGD", std::make_unique<machine_instruction>("VCGD", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U }, 48, 1609)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VCLGD", std::make_unique<machine_instruction>("VCLGD", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U }, 48, 1611)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFD", std::make_unique<machine_instruction>("VFD", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1613)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFI", std::make_unique<machine_instruction>("VFI", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U }, 48, 1615)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFLL", std::make_unique<machine_instruction>("VFLL", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1617)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFLR", std::make_unique<machine_instruction>("VFLR", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U }, 48, 1618)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFMAX", std::make_unique<machine_instruction>("VFMAX", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U }, 48, 1619)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFMIN", std::make_unique<machine_instruction>("VFMIN", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U }, 48, 1625)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFM", std::make_unique<machine_instruction>("VFM", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1631)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFMA", std::make_unique<machine_instruction>("VFMA", mach_format::VRR_e, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1633)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFMS", std::make_unique<machine_instruction>("VFMS", mach_format::VRR_e, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1633)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFNMA", std::make_unique<machine_instruction>("VFNMA", mach_format::VRR_e, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1633)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFNMS", std::make_unique<machine_instruction>("VFNMS", mach_format::VRR_e, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1633)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFPSO", std::make_unique<machine_instruction>("VFPSO", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U, mask_4_U }, 48, 1635)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFSQ", std::make_unique<machine_instruction>("VFSQ", mach_format::VRR_a, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1636)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFS", std::make_unique<machine_instruction>("VFS", mach_format::VRR_c, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, mask_4_U, mask_4_U }, 48, 1637)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VFTCI", std::make_unique<machine_instruction>("VFTCI", mach_format::VRI_e, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, imm_12_S, mask_4_U, mask_4_U }, 48, 1638)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VAP", std::make_unique<machine_instruction>("VAP", mach_format::VRI_f, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, imm_8_U, mask_4_U }, 48, 1643)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VCP", std::make_unique<machine_instruction>("VCP", mach_format::VRR_h, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1644)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VCVB", std::make_unique<machine_instruction>("VCVB", mach_format::VRR_i, std::vector<machine_operand_format>{reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1645)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VCVBG", std::make_unique<machine_instruction>("VCVBG", mach_format::VRR_i, std::vector<machine_operand_format>{reg_4_U, vec_reg_4_U, mask_4_U }, 48, 1645)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VCVD", std::make_unique<machine_instruction>("VCVD", mach_format::VRI_i, std::vector<machine_operand_format>{vec_reg_4_U, reg_4_U, imm_8_S, mask_4_U }, 48, 1646)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VCVDG", std::make_unique<machine_instruction>("VCVDG", mach_format::VRI_i, std::vector<machine_operand_format>{vec_reg_4_U, reg_4_U, imm_8_S, mask_4_U }, 48, 1646)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VDP", std::make_unique<machine_instruction>("VDP", mach_format::VRI_f, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, imm_8_U, mask_4_U }, 48, 1648)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VMP", std::make_unique<machine_instruction>("VMP", mach_format::VRI_f, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, imm_8_U, mask_4_U }, 48, 1650)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VMSP", std::make_unique<machine_instruction>("VMSP", mach_format::VRI_f, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, imm_8_U, mask_4_U }, 48, 1651)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VRP", std::make_unique<machine_instruction>("VRP", mach_format::VRI_f, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, imm_8_U, mask_4_U }, 48, 1654)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSDP", std::make_unique<machine_instruction>("VSDP", mach_format::VRI_f, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, imm_8_U, mask_4_U }, 48, 1656)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSP", std::make_unique<machine_instruction>("VSP", mach_format::VRI_f, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, vec_reg_4_U, imm_8_U, mask_4_U }, 48, 1658)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VLIP", std::make_unique<machine_instruction>("VLIP", mach_format::VRI_h, std::vector<machine_operand_format>{vec_reg_4_U, imm_16_S , imm_4_U }, 48, 1649)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VPKZ", std::make_unique<machine_instruction>("VPKZ", mach_format::VSI, std::vector<machine_operand_format>{vec_reg_4_U, db_12_4_U, imm_8_U }, 48, 1652)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VPSOP", std::make_unique<machine_instruction>("VPSOP", mach_format::VRI_g, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, imm_8_U, imm_8_U, mask_4_U }, 48, 1653)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VSRP", std::make_unique<machine_instruction>("VSRP", mach_format::VRI_g, std::vector<machine_operand_format>{vec_reg_4_U, vec_reg_4_U, imm_8_U, imm_8_S, mask_4_U }, 48, 1657)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VTP", std::make_unique<machine_instruction>("VTP", mach_format::VRR_g, std::vector<machine_operand_format>{vec_reg_4_U }, 48, 1660)));
	result.insert(std::pair <const std::string, machine_instruction_ptr>("VUPKZ", std::make_unique<machine_instruction>("VUPKZ", mach_format::VSI, std::vector<machine_operand_format>{vec_reg_4_U, db_12_4_U, imm_8_U }, 48, 1660)));
	return result;
}

std::map<const std::string, mnemonic_code> hlasm_plugin::parser_library::context::instruction::get_mnemonic_codes()
{
	std::map<const std::string, mnemonic_code> result;
	result.insert(std::make_pair<const std::string, mnemonic_code>("B", { "BC", { {0,15} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BR", { "BCR", { {0,15} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("J", { "BRC", { {0,15} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("NOP", { "BC", { {0, 0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("NOPR", { "BCR", { {0, 0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JNOP", { "BRC", { {0, 0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BH", { "BC", { {0,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BHR", { "BCR", { {0,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JH", { "BRC", { {0,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BL", { "BC", { {0,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BLR", { "BCR", { {0,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JL", { "BRC", { {0,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BE", { "BC", { {0,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BER", { "BCR", { {0,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JE", { "BRC", { {0,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BNH", { "BC", { {0,13} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BNHR", { "BCR", { {0,13} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JNH", { "BRC", { {0,13} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BNL", { "BC", { {0,11} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BNLR", { "BCR", { {0,11} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JNL", { "BRC", { {0,11} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BNE", { "BC", { {0,7} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BNER", { "BCR", { {0,7} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JNE", { "BRC", { {0,7} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BP", { "BC", { {0,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BPR", { "BCR", { {0,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JP", { "BRC", { {0,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BM", { "BC", { {0,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BMR", { "BCR", { {0,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JM", { "BRC", { {0,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BZ", { "BC", { {0,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BZR", { "BCR", { {0,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JZ", { "BRC", { {0,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BO", { "BC", { {0,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BOR", { "BCR", { {0,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JO", { "BRC", { {0,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BNP", { "BC", { {0,13} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BNPR", { "BCR", { {0,13} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JNP", { "BRC", { {0,13} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BNM", { "BC", { {0,11} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BNMR", { "BCR", { {0,11} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JNM", { "BRC", { {0,11} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BNZ", { "BC", { {0,7} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BNZR", { "BCR", { {0,7} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JNZ", { "BRC", { {0,7} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BNO", { "BC", { {0,14} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BNOR", { "BCR", { {0,14} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JNO", { "BRC", { {0,14} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BO", { "BC", { {0,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BOR", { "BCR", { {0,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BM", { "BC", { {0,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BMR", { "BCR", { {0,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BZ", { "BC", { {0,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BZR", { "BCR", { {0,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BNO", { "BC", { {0,14} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BNOR", { "BCR", { {0,14} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BNM", { "BC", { {0,11} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BNMR", { "BCR", { {0,11} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BNZ", { "BC", { {0,7} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BNZR", { "BCR", { {0,7} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRUL", { "BRCL", { {0,15} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRHL", { "BRCL", { {0,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRLL", { "BRCL", { {0,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BREL", { "BRCL", { {0,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRNHL", { "BRCL", { {0,13} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRNLL", { "BRCL", { {0,11} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRNEL", { "BRCL", { {0,7} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRPL", { "BRCL", { {0,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRML", { "BRCL", { {0,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRZL", { "BRCL", { {0,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BROL", { "BRCL", { {0,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRNPL", { "BRCL", { {0,13} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRNML", { "BRCL", { {0,11} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRNZL", { "BRCL", { {0,7} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRNOL", { "BRCL", { {0,14} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRO", { "BRC", { {0,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRP", { "BRC", { {0,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRH", { "BRC", { {0,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRL", { "BRC", { {0,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRM", { "BRC", { {0,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRNE", { "BRC", { {0,7} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRNZ", { "BRC", { {0,7} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRE", { "BRC", { {0,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRZ", { "BRC", { {0,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRNL", { "BRC", { {0,11} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRNM", { "BRC", { {0,11} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRNH", { "BRC", { {0,13} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRNP", { "BRC", { {0,13} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRNO", { "BRC", { {0,14} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BRU", { "BRC", { {0,15} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JLU", { "BRCL", { {0,15} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JLNOP", { "BRCL", { {0,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JLH", { "BRCL", { {0,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JLL", { "BRCL", { {0,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JLE", { "BRCL", { {0,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JLNH", { "BRCL", { {0,13} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JLNL", { "BRCL", { {0,11} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JLNE", { "BRCL", { {0,7} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JLP", { "BRCL", { {0,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JLM", { "BRCL", { {0,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JLZ", { "BRCL", { {0,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JLO", { "BRCL", { {0,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JLNP", { "BRCL", { {0,13} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JLNM", { "BRCL", { {0,11} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JLNZ", { "BRCL", { {0,7} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("JLNO", { "BRCL", { {0,14} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BIO", { "BIC", { {0,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BIP", { "BIC", { {0,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BIH", { "BIC", { {0,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BIM", { "BIC", { {0,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BIL", { "BIC", { {0,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BINZ", { "BIC", { {0,7} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BINE", { "BIC", { {0,7} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BIZ", { "BIC", { {0,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BIE", { "BIC", { {0,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BINM", { "BIC", { {0,11} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BINL", { "BIC", { {0,11} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BINP", { "BIC", { {0,13} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BINH", { "BIC", { {0,13} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BINO", { "BIC", { {0,14} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("BI", { "BIC", { {0,15} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VZERO", { "VGBM", { {0,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VONE", { "VGBM", { {1,65535} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VGMB", { "VGM", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VGMH", { "VGM", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VGMF", { "VGM", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VGMG", { "VGM", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLREPB", { "VLREP", { {2,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLREPH", { "VLREP", { {2,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLREPF", { "VLREP", { {2,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLREPG", { "VLREP", { {2,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLGVB", { "VLGV", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLGVH", { "VLGV", { {3,1}} }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLGVF", { "VLGV", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLGVG", { "VLGV", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLLEZB", { "VLLEZ", { {2,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLLEZH", { "VLLEZ", { {2,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLLEZF", { "VLLEZ", { {2,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLLEZG", { "VLLEZ", { {2,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLLEZLF", { "VLLEZ", { {2,6} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLVGB", { "VLVG", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLVGH", { "VLVG", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLVGF", { "VLVG", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLVGG", { "VLVG", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMRHB", { "VMRH", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMRHH", { "VMRH", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMRHF", { "VMRH", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMRHG", { "VMRH", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMRLB", { "VMRL", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMRLH", { "VMRL", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMRLF", { "VMRL", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMRLG", { "VMRL", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VPKSH", { "VPKS", { {3,1}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VPKSF", { "VPKS", { {3,2}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VPKSG", { "VPKS", { {3,3}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VPKSHS", { "VPKS", { {3,1}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VPKSFS", { "VPKS", { {3,2}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VPKSGS", { "VPKS", { {3,3}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VPKLSH", { "VPKLS", { {3,1}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VPKLSF", { "VPKLS", { {3,2}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VPKLSG", { "VPKLS", { {3,3}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VPKLSHS", { "VPKLS", { {3,1}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VPKLSFS", { "VPKLS", { {3,2}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VPKLSGS", { "VPKLS", { {3,3}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VREPB", { "VREP", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VREPH", { "VREP", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VREPF", { "VREP", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VREPG", { "VREP", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VREPIB", { "VREPI", { {2,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VREPIH", { "VREPI", { {2,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VREPIF", { "VREPI", { {2,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VREPIG", { "VREPI", { {2,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSEGB", { "VSEG", { {2,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSEGH", { "VSEG", { {2,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSEGF", { "VSEG", { {2,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VUPHB", { "VUPH", { {2,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VUPHH", { "VUPH", { {2,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VUPHF", { "VUPH", { {2,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VUPLHB", { "VUPLH", { {2,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VUPLHG", { "VUPLH", { {2,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VUPLHF", { "VUPLH", { {2,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VUPLB", { "VUPL", { {2,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VUPLHW", { "VUPL", { {2,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VUPLF", { "VUPL", { {2,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VUPLLB", { "VUPLL", { {2,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VUPLLH", { "VUPLL", { {2,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VUPLLF", { "VUPLL", { {2,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VAB", { "VA", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VAH", { "VA", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VAF", { "VA", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VAG", { "VA", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VAQ", { "VA", { {3,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VACCB", { "VACC", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VACCH", { "VACC", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VACCF", { "VACC", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VACCG", { "VACC", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VACCQ", { "VACC", { {3,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VACQ", { "VAC", { {3,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VACCCQ", { "VACCC", { {3,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VAVGB", { "VAVG", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VAVGH", { "VAVG", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VAVGF", { "VAVG", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VAVGG", { "VAVG", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VAVGLB", { "VAVGL", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VAVGLH", { "VAVGL", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VAVGLF", { "VAVGL", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VAVGLG", { "VAVGL", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VECB", { "VEC", { {2,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VECH", { "VEC", { {2,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VECF", { "VEC", { {2,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VECG", { "VEC", { {2,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VECLB", { "VECL", { {2,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VECLH", { "VECL", { {2,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VECLF", { "VECL", { {2,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VECLG", { "VECL", { {2,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCEQB", { "VCEQ", { {3,0}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCEQH", { "VCEQ", { {3,1}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCEQF", { "VCEQ", { {3,2}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCEQG", { "VCEQ", { {3,3}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCEQBS", { "VCEQ", { {3,0}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCEQHS", { "VCEQ", { {3,1}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCEQFS", { "VCEQ", { {3,2}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCEQGS", { "VCEQ", { {3,3}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCHB", { "VCH", { {3,0}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCHH", { "VCH", { {3,1}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCHF", { "VCH", { {3,2}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCHG", { "VCH", { {3,3}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCHBS", { "VCH", { {3,0}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCHHS", { "VCH", { {3,1}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCHFS", { "VCH", { {3,2}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCHGS", { "VCH", { {3,3}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCHLB", { "VCHL", { {3,0}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCHLH", { "VCHL", { {3,1}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCHLF", { "VCHL", { {3,2}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCHLG", { "VCHL", { {3,3}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCHLBS", { "VCHL", { {3,0}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCHLHS", { "VCHL", { {3,1}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCHLFS", { "VCHL", { {3,2}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCHLGS", { "VCHL", { {3,3}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCLZB", { "VCLZ", { {2,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCLZH", { "VCLZ", { {2,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCLZF", { "VCLZ", { {2,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCLZG", { "VCLZ", { {2,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VGFMB", { "VGFM", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VGFMB", { "VGFM", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VGFMB", { "VGFM", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VGFMB", { "VGFM", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VGFMAB", { "VGFMA", { {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VGFMAH", { "VGFMA", { {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VGFMAF", { "VGFMA", { {4,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VGFMAG", { "VGFMA", { {4,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLCB", { "VLC", { {2,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLCH", { "VLC", { {2,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLCF", { "VLC", { {2,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLCG", { "VLC", { {2,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLPB", { "VLP", { {2,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLPH", { "VLP", { {2,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLPF", { "VLP", { {2,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLPG", { "VLP", { {2,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMXB", { "VMX", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMXH", { "VMX", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMXF", { "VMX", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMXG", { "VMX", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMXLB", { "VMXL", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMXLH", { "VMXL", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMXLF", { "VMXL", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMXLG", { "VMXL", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMNB", { "VMN", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMNH", { "VMN", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMNF", { "VMN", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMNG", { "VMN", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMNLB", { "VMNL", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMNLH", { "VMNL", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMNLF", { "VMNL", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMNLG", { "VMNL", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMALB", { "VMAL", { {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMALHW", { "VMAL", { {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMALF", { "VMAL", { {4,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMAHB", { "VMAH", { {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMAHH", { "VMAH", { {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMAHF", { "VMAH", { {4,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMALHB", { "VMALH", { {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMALHH", { "VMALH", { {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMALHF", { "VMALH", { {4,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMAEB", { "VMAE", { {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMAEH", { "VMAE", { {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMAEF", { "VMAE", { {4,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMALEB", { "VMALE", { {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMALEH", { "VMALE", { {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMALEF", { "VMALE", { {4,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMAOB", { "VMAO", { {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMAOH", { "VMAO", { {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMAOF", { "VMAO", { {4,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMALOB", { "VMALO", { {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMALOH", { "VMALO", { {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMALOF", { "VMALO", { {4,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMHB", { "VMH", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMHH", { "VMH", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMHF", { "VMH", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMLHB", { "VMLH", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMLHH", { "VMLH", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMLHF", { "VMLH", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMLB", { "VML", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMLHW", { "VML", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMLF", { "VML", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMEB", { "VME", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMEH", { "VME", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMEF", { "VME", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMLEB", { "VMLE", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMLEH", { "VMLE", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMLEF", { "VMLE", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMSLG", { "VMSL", { {4,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMOB", { "VMO", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMOH", { "VMO", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMOF", { "VMO", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMLOB", { "VMLO", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMLOH", { "VMLO", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VMLOF", { "VMLO", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VPOPCTB", { "VPOPCT", { {2,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VPOPCTH", { "VPOPCT", { {2,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VPOPCTF", { "VPOPCT", { {2,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VPOPCTG", { "VPOPCT", { {2,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VERLLVB", { "VERLLV", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VERLLVH", { "VERLLV", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VERLLVF", { "VERLLV", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VERLLVG", { "VERLLV", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VERLLB", { "VERLL", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VERLLH", { "VERLL", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VERLLF", { "VERLL", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VERLLG", { "VERLL", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VERIMB", { "VERIM", { {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VERIMH", { "VERIM", { {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VERIMF", { "VERIM", { {4,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VERIMG", { "VERIM", { {4,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESLVB", { "VESLV", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESLVH", { "VESLV", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESLVF", { "VESLV", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESLVG", { "VESLV", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESLB", { "VESL", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESLH", { "VESL", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESLF", { "VESL", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESLG", { "VESL", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESRAVB", { "VESRAV", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESRAVH", { "VESRAV", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESRAVF", { "VESRAV", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESRAVG", { "VESRAV", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESRAB", { "VESRA", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESRAH", { "VESRA", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESRAF", { "VESRA", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESRAG", { "VESRA", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESRLVB", { "VESRLV", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESRLVH", { "VESRLV", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESRLVF", { "VESRLV", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESRLVG", { "VESRLV", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESRLB", { "VESRL", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESRLH", { "VESRL", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESRLF", { "VESRL", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VESRLG", { "VESRL", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSB", { "VS", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSH", { "VS", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSF", { "VS", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSG", { "VS", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSQ", { "VS", { {3,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSCBIB", { "VSCBI", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSCBIH", { "VSCBI", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSCBIF", { "VSCBI", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSCBIG", { "VSCBI", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSCBIQ", { "VSCBI", { {3,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSBIQ", { "VSBI", { {4,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSBCBIQ", { "VSBCBI", { {4,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSUMQF", { "VSUMQ", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSUMQG", { "VSUMQ", { {3,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSUMGH", { "VSUMG", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSUMGF", { "VSUMG", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSUMB", { "VSUM", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSUMH", { "VSUM", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFAEB", { "VFAE", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFAEH", { "VFAE", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFAEF", { "VFAE", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFEEB", { "VFEE", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFEEH", { "VFEE", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFEEF", { "VFEE", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFEEBS", { "VFEE", { {3,0}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFEEGS", { "VFEE", { {3,1}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFEEFS", { "VFEE", { {3,2}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFEEZB", { "VFEE", { {3,0}, {4,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFEEZH", { "VFEE", { {3,1}, {4,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFEEZF", { "VFEE", { {3,2}, {4,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFEEZBS", { "VFEE", { {3,0}, {4,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFEEZHS", { "VFEE", { {3,1}, {4,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFEEZFS", { "VFEE", { {3,2}, {4,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFENEB", { "VFENE", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFENEH", { "VFENE", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFENEF", { "VFENE", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFENEBS", { "VFENE", { {3,0}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFENEHS", { "VFENE", { {3,1}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFENEFS", { "VFENE", { {3,2}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFENEZB", { "VFENE", { {3,0}, {4,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFENEZH", { "VFENE", { {3,1}, {4,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFENEZF", { "VFENE", { {3,2}, {4,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFENEZBS", { "VFENE", { {3,0}, {4,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFENEZHS", { "VFENE", { {3,1}, {4,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFENEZFS", { "VFENE", { {3,2}, {4,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VISTRB", { "VISTR", { {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VISTRH", { "VISTR", { {3,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VISTRF", { "VISTR", { {3,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VISTRBS", { "VISTR", { {3,0}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VISTRHS", { "VISTR", { {3,1}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VISTRFS", { "VISTR", { {3,2}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSTRCB", { "VSTRC", { {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSTRCH", { "VSTRC", { {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSTRCF", { "VSTRC", { {4,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFASB", { "VFA", { {3,2}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFADB", { "VFA", { {3,3}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFASB", { "VFA", { {3,2}, {4,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFADB", { "VFA", { {3,3}, {4,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFAXB", { "VFA", { {3,4}, {4,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFCSB", { "WFC", { {3,2}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFCDB", { "WFC", { {3,3}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFCXB", { "WFC", { {3,4}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFKSB", { "WFK", { {3,2}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFKDB", { "WFK", { {3,3}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFKXB", { "WFK", { {3,4}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFCESB", { "VFCE", { {3,2}, {4,0}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFCESBS", { "VFCE", { {3,2}, {4,0}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFCEDB", { "VFCE", { {3,3}, {4,0}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFCEDBS", { "VFCE", { {3,3}, {4,0}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFCESB", { "VFCE", { {3,2}, {4,8}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFCESBS", { "VFCE", { {3,2}, {4,8}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFCEDB", { "VFCE", { {3,3}, {4,8}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFCEDBS", { "VFCE", { {3,3}, {4,8}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFCEXB", { "VFCE", { {3,4}, {4,8}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFCEXBS", { "VFCE", { {3,4}, {4,8}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFKESB", { "VFCE", { {3,2}, {4,4}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFKESBS", { "VFCE", { {3,2}, {4,4}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFKEDB", { "VFCE", { {3,3}, {4,4}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFKEDBS", { "VFCE", { {3,3}, {4,4}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFKESB", { "VFCE", { {3,2}, {4,12}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFKESBS", { "VFCE", { {3,2}, {4,12}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFKEDB", { "VFCE", { {3,3}, {4,12}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFKEDBS", { "VFCE", { {3,3}, {4,12}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFKEXB", { "VFCE", { {3,4}, {4,12}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFKEXBS", { "VFCE", { {3,4}, {4,12}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFCHSB", { "VFCH", { {3,2}, {4,0}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFCHSBS", { "VFCH", { {3,2}, {4,0}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFCHDB", { "VFCH", { {3,3}, {4,0}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFCHDBS", { "VFCH", { {3,3}, {4,0}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFCHSB", { "VFCH", { {3,2}, {4,8}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFCHSBS", { "VFCH", { {3,2}, {4,8}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFCHDB", { "VFCH", { {3,3}, {4,8}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFCHDBS", { "VFCH", { {3,3}, {4,8}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFCHXB", { "VFCH", { {3,4}, {4,8}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFCHXBS", { "VFCH", { {3,4}, {4,8}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFKHSB", { "VFCH", { {3,2}, {4,4}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFKHSBS", { "VFCH", { {3,2}, {4,4}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFKHDB", { "VFCH", { {3,3}, {4,4}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFKHDBS", { "VFCH", { {3,3}, {4,4}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFKHSB", { "VFCH", { {3,2}, {4,12}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFKHSBS", { "VFCH", { {3,2}, {4,12}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFKHDB", { "VFCH", { {3,3}, {4,12}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFKHDBS", { "VFCH", { {3,3}, {4,12}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFKHXB", { "VFCH", { {3,4}, {4,12}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFKHXBS", { "VFCH", { {3,4}, {4,12}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFCHESB", { "VFCHE", { {3,2}, {4,0}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFCHESBS", { "VFCHE", { {3,2}, {4,0}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFCHEDB", { "VFCHE", { {3,3}, {4,0}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFCHEDBS", { "VFCHE", { {3,3}, {4,0}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFCHSB", { "VFCHE", { {3,2}, {4,8}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFCHESBS", { "VFCHE", { {3,2}, {4,8}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFCHEDB", { "VFCHE", { {3,3}, {4,8}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFCHEDBS", { "VFCHE", { {3,3}, {4,8}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFCHEXB", { "VFCHE", { {3,4}, {4,8}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFCHEXBS", { "VFCHE", { {3,4}, {4,8}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFKHESB", { "VFCHE", { {3,2}, {4,4}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFKHESBS", { "VFCHE", { {3,2}, {4,4}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFKHEDB", { "VFCHE", { {3,3}, {4,4}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFKHEDBS", { "VFCHE", { {3,3}, {4,4}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFKHESB", { "VFCHE", { {3,2}, {4,12}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFKHESBS", { "VFCHE", { {3,2}, {4,12}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFKHEDB", { "VFCHE", { {3,3}, {4,12}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFKHEDBS", { "VFCHE", { {3,3}, {4,12}, {5,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFKHEXB", { "VFCHE", { {3,4}, {4,12}, {5,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCDGB", { "VCDG", { {2,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCDLGB", { "VCDLG", { {2,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCGDB", { "VCGD", { {2,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VCLGDB", { "VCLGD", { {2,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFDSB", { "VFD", { {3,2}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFDSB", { "VFD", { {3,2}, {4,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFDDB", { "VFD", { {3,3}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFDDB", { "VFD", { {3,3}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFDDB", { "VFD", { {3,3}, {4,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFDXB", { "VFD", { {3,4}, {4,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFISB", { "VFI", { {2,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFIDB", { "VFI", { {2,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLDE", { "VFLL", { } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLDEB", { "VFLL", { {2,2}, {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WLDEB", { "VFLL", { {2,2}, {3,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFLLS", { "VFLL", { {2,2}, {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFLLS", { "VFLL", { {2,2}, {3,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFLLD", { "VFLL", { {2,3}, {3,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLED", { "VFLR", { } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VLEDB", { "VFLR", { {2,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFLRD", { "VFLR", { {2,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFMAXSB", { "VFMAX", { {3,2}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFMAXDB", { "VFMAX", { {3,3}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFMAXSB", { "VFMAX", { {3,2}, {4,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFMAXDB", { "VFMAX", { {3,3}, {4,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFMAXXB", { "VFMAX", { {3,4}, {4,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFMINSB", { "VFMIN", { {3,2}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFMINDB", { "VFMIN", { {3,3}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFMINSB", { "VFMIN", { {3,2}, {4,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFMINDB", { "VFMIN", { {3,3}, {4,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFMINXB", { "VFMIN", { {3,4}, {4,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFMSB", { "VFM", { {3,2}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFMDB", { "VFM", { {3,3}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFMSB", { "VFM", { {3,2}, {4,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFMDB", { "VFM", { {3,3}, {4,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFMXB", { "VFM", { {3,4}, {4,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFMASB", { "VFMA", { {4,0}, {5,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFMADB", { "VFMA", { {4,0}, {5,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFMASB", { "VFMA", { {4,8}, {5,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFMADB", { "VFMA", { {4,8}, {5,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFMAXB", { "VFMA", { {4,8}, {5,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFMSSB", { "VFMS", { {4,0}, {5,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFMSDB", { "VFMS", { {4,0}, {5,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFMSSB", { "VFMS", { {4,8}, {5,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFMSDB", { "VFMS", { {4,8}, {5,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFMSXB", { "VFMS", { {4,8}, {5,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFNMASB", { "VFNMA", { {4,0}, {5,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFNMADB", { "VFNMA", { {4,0}, {5,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFNMASB", { "VFNMA", { {4,8}, {5,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFNMADB", { "VFNMA", { {4,8}, {5,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFNMAXB", { "VFNMA", { {4,8}, {5,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFNMSSB", { "VFNMS", { {4,0}, {5,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFNMSDB", { "VFNMS", { {4,0}, {5,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFNMSSB", { "VFNMS", { {4,8}, {5,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFNMSDB", { "VFNMS", { {4,8}, {5,3} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFNMSXB", { "VFNMS", { {4,8}, {5,4} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFPSOSB", { "VFPSO", { {2,2}, {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFPSOSB", { "VFPSO", { {2,2}, {3,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFLCSB", { "VFPSO", { {2,2}, {3,0}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFLCSB", { "VFPSO", { {2,2}, {3,8}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFLNSB", { "VFPSO", { {2,2}, {3,0}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFLNSB", { "VFPSO", { {2,2}, {3,8}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFLPSB", { "VFPSO", { {2,2}, {3,0}, {4,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFLPSB", { "VFPSO", { {2,2}, {3,8}, {4,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFPSODB", { "VFPSO", { {2,3}, {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFPSODB", { "VFPSO", { {2,3}, {3,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFLCDB", { "VFPSO", { {2,3}, {3,0}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFLCDB", { "VFPSO", { {2,3}, {3,8}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFLNDB", { "VFPSO", { {2,3}, {3,0}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFLNDB", { "VFPSO", { {2,3}, {3,8}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFLPDB", { "VFPSO", { {2,3}, {3,0}, {4,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFLPDB", { "VFPSO", { {2,3}, {3,8}, {4,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFPSOXB", { "VFPSO", { {2,4}, {3,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFLCXB", { "VFPSO", { {2,4}, {3,8}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFLNXB", { "VFPSO", { {2,4}, {3,8}, {4,1} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFLPXB", { "VFPSO", { {2,4}, {3,8}, {4,2} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFSQSB", { "VFSQ", { {2,2}, {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFSQDB", { "VFSQ", { {2,3}, {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFSQSB", { "VFSQ", { {2,2}, {3,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFSQDB", { "VFSQ", { {2,3}, {3,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFSQXB", { "VFSQ", { {2,4}, {3,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFSSB", { "VFS", { {2,2}, {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFSDB", { "VFS", { {2,3}, {3,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFSSB", { "VFS", { {2,2}, {3,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFSDB", { "VFS", { {2,3}, {3,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFSXB", { "VFS", { {2,4}, {3,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFTCISB", { "VFTCI", { {3,2}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFTCIDB", { "VFTCI", { {3,3}, {4,0} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFTCISB", { "VFTCI", { {3,2}, {4,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFTCIDB", { "VFTCI", { {3,3}, {4,8} } }));
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFTCIXB", { "VFTCI", { {3,4}, {4,8} } }));
	// instruction under this position contain an OR operation not marked in this list

	// in case the operand is ommited, the OR number should be assigned to the value of the ommited operand
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFAEBS", { "VFAE", { {3,0} } })); // operand with index 4 ORed with 1
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFAEHS", { "VFAE", { {3,1} } })); // operand with index 4 ORed with 1
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFAEFS", { "VFAE", { {3,2} } })); // operand with index 4 ORed with 1
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFAEZB", { "VFAE", { {3,0} } })); // operand with index 4 ORed with 2
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFAEZH", { "VFAE", { {3,1} } })); // operand with index 4 ORed with 2
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFAEZF", { "VFAE", { {3,2} } })); // operand with index 4 ORed with 2
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFAEZBS", { "VFAE", { {3,0} } })); // operand with index 4 ORed with 3
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFAEZHS", { "VFAE", { {3,1} } })); // operand with index 4 ORed with 3
	result.insert(std::make_pair<const std::string, mnemonic_code>("VFAEZFS", { "VFAE", { {3,2} } })); // operand with index 4 ORed with 3
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSTRCBS", { "VSTRC", { {4,0} } })); // operand with index 5 ORed with 1
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSTRCHS", { "VSTRC", { {4,1} } })); // operand with index 5 ORed with 1
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSTRCFS", { "VSTRC", { {4,2} } })); // operand with index 5 ORed with 1
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSTRCZB", { "VSTRC", { {4,0} } })); // operand with index 5 ORed with 2
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSTRCZH", { "VSTRC", { {4,1} } })); // operand with index 5 ORed with 2
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSTRCZF", { "VSTRC", { {4,2} } })); // operand with index 5 ORed with 2
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSTRCZBS", { "VSTRC", { {4,0} } })); // operand with index 5 ORed with 3
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSTRCZHS", { "VSTRC", { {4,1} } })); // operand with index 5 ORed with 3
	result.insert(std::make_pair<const std::string, mnemonic_code>("VSTRCZFS", { "VSTRC", { {4,2} } })); // operand with index 5 ORed with 3
	 // always OR
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFISB", { "VFI", { {2,2} } })); // operand with index 3 ORed with 8
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFIDB", { "VFI", { {2,3} } })); // operand with index 3 ORed with 8
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFIXB", { "VFI", { {2,4} } })); // operand with index 3 ORed with 8
	result.insert(std::make_pair<const std::string, mnemonic_code>("WCDGB", { "VCDG", { {2,3} } })); // operand with index 3 ORed with 8
	result.insert(std::make_pair<const std::string, mnemonic_code>("WCDLGB", { "VCDLG", { {2,3} } })); // operand with index 3 ORed with 8
	result.insert(std::make_pair<const std::string, mnemonic_code>("WCGDB", { "VCGD", { {2,3} } })); // operand with index 3 ORed with 8
	result.insert(std::make_pair<const std::string, mnemonic_code>("WCLGDB", { "VCLGD", { {2,3} } })); // operand with index 3 ORed with 8
	result.insert(std::make_pair<const std::string, mnemonic_code>("WLEDB", { "VFLR", { {2,3} } })); // operand with index 3 ORed with 8
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFLRD", { "VFLR", { {2,3} } })); // operand with index 3 ORed with 8
	result.insert(std::make_pair<const std::string, mnemonic_code>("WFLRX", { "VFLR", { {2,4} } })); // operand with index 3 ORed with 8
	return result;
}

std::map<const std::string, machine_instruction_ptr> instruction::machine_instructions = instruction::get_machine_instructions();

const std::map<const std::string, mnemonic_code> instruction::mnemonic_codes = instruction::get_mnemonic_codes();