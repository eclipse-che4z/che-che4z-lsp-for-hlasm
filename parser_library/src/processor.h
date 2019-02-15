#ifndef HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_H
#define HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_H

#include <memory>

#include "context/hlasm_context.h"
#include "shared/lexer.h"
#include "semantics/semantic_info.h"
#include "generated/hlasmparser.h"
#include "diagnosable.h"
#include "file.h"




namespace hlasm_plugin::parser_library {


using program_context = generated::hlasmparser::ProgramContext;

class parse_lib_provider
{
public:
	
	virtual program_context * parse_library(const std::string & caller, const std::string & library, std::shared_ptr<context::hlasm_context> ctx) = 0;
};


class processor : public virtual diagnosable
{
public:
	virtual bool parse_info_updated() = 0;

	//starts parser with new (empty) context
	virtual program_context * parse(parse_lib_provider &) = 0;
	//starts parser with in the context of parameter
	virtual program_context * parse(parse_lib_provider &, std::shared_ptr<context::hlasm_context>) = 0;
	
	virtual semantics::semantic_info & semantic_info() = 0;
};

class processor_file : public virtual file, public processor
{
};

}
#endif