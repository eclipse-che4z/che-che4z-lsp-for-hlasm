#ifndef HLASMPLUGIN_PARSERLIBRARY_ASSEMBLER_INSTRUCTION_H
#define HLASMPLUGIN_PARSERLIBRARY_ASSEMBLER_INSTRUCTION_H
#include <iomanip>
#include <string>

#include "instr_class.h"

namespace hlasm_plugin
{
namespace parser_library
{
namespace checking
{
/*
TO DO - notes
- dxd, dc, ds instructions - if evaluation and control takes places before checker, these instructions need only one class for all of them and need to just check complexity - TO DO
- operand evaluation before passing arguments to this class
- parameters to class need to be already parsed in a vector
- all parameters passed to class need to be uppercase
*/

class xattr : public assembler_instruction
{
public:
	xattr(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

class using_instr : public assembler_instruction
{
public:
	using_instr(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

class title : public assembler_instruction
{
public:
	title(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

class rmode : public assembler_instruction
{
public:
	rmode(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

class punch : public assembler_instruction
{
public:
	punch(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

class print : public assembler_instruction
{
public:
	print(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

class stack_instr : public assembler_instruction
{
public:
	stack_instr(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

class org : public assembler_instruction
{
public:
	org(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

// class for opsyn instruction, TO DO - operation code checker (need to be previously defined)
class opsyn : public assembler_instruction
{
public:
	opsyn(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

class mnote : public assembler_instruction
{
public:
	mnote(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

class iseq : public assembler_instruction
{
public:
	iseq(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

class ictl : public assembler_instruction
{
public:
	ictl(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

// class for external instructions (extrn and wxtrn), TO DO check external symbols
class external : public assembler_instruction
{
public:
	external(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

class exitctl : public assembler_instruction
{
public:
	exitctl(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

// class for equ instruction, TO DO - check value? 
class equ : public assembler_instruction
{
public:
	equ(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

// class for entry instruction, TO DO - check entry point symbols
class entry : public assembler_instruction
{
public:
	entry(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);;
	bool check(const std::vector<const one_operand*> & to_check);
};

class end : public assembler_instruction
{
public:
	end(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

class drop : public assembler_instruction
{
public:
	drop(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

// class for data instructions - ds, dc, dxd -  checking operands before this class, TO DO
class data : public assembler_instruction
{
public:
	data(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

// class for copy instruction, TO DO - parse member
class copy : public assembler_instruction
{
public:
	copy(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

class cnop : public assembler_instruction
{
public:
	cnop(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

// class for ccw (and ccw0, ccw1) instruction, operands can be expressions, TO DO
class ccw : public assembler_instruction
{
public:
	ccw(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

// class for instruction requiring only one expression_instruction (defining for example number of lines) - ceject, space and start instr
class expression_instruction : public assembler_instruction
{
public:
	expression_instruction(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

class cattr : public assembler_instruction
{
public:
	cattr(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

class amode : public assembler_instruction
{
public:
	amode(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

class alias : public assembler_instruction
{
public:
	alias(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

class ainsert : public assembler_instruction
{
public:
	ainsert(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

class acontrol : public assembler_instruction
{
public:
	acontrol(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

class adata : public assembler_instruction
{
public:
	adata(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

// class for instructions without operands
class no_operands : public assembler_instruction
{
public:
	no_operands(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

// class for process instruction
class process : public assembler_instruction
{
public:
	process(const std::vector<label_types>& allowed_types, const std::string& name_of_instruction);
	bool check(const std::vector<const one_operand*> & to_check);
};

}
}
}

#endif
