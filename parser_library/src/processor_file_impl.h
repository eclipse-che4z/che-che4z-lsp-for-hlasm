#ifndef HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_FILE_H
#define HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_FILE_H

#include "processor.h"
#include "file_impl.h"
#include "parser_error_listener.h"
#include "shared/token_stream.h"
#include "error_strategy.h"

namespace hlasm_plugin::parser_library {

class processor_file_impl : public virtual file_impl, public virtual processor_file
{
public:
	processor_file_impl(std::string file_uri);
	processor_file_impl(file_impl &&);
	void collect_diags() const override;
	bool is_once_only() const override;

	//starts parser with new (empty) context
	program_context * parse(parse_lib_provider &);
	//starts parser with in the context of parameter
	program_context * parse(parse_lib_provider &, std::shared_ptr<context::hlasm_context>);

	semantics::semantic_info & semantic_info();

	bool parse_info_updated() override;

	virtual ~processor_file_impl() = default;
private:
	parser_error_listener listener_;

	std::unique_ptr<input_source> input_;
	std::unique_ptr <lexer> lexer_;
	std::unique_ptr <token_stream> tokens_;
	std::unique_ptr <generated::hlasmparser> parser_;

	semantics::semantic_info sm_info_;
	bool parse_info_updated_ = false;
};

}
#endif // !HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_FILE_H
