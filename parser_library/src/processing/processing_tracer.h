#ifndef HLASMPLUGIN_PARSERLIBRARY_PROCESSING_TRACER_H
#define HLASMPLUGIN_PARSERLIBRARY_PROCESSING_TRACER_H

namespace hlasm_plugin::parser_library::processing
{

class processing_tracer
{
public:
	virtual void statement(range statement_range) = 0;
	virtual ~processing_tracer() = default;
};

}

#endif