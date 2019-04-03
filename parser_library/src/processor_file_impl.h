#ifndef HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_FILE_H
#define HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_FILE_H

#include "processor.h"
#include "file_impl.h"
#include "analyzer.h"

namespace hlasm_plugin::parser_library {

class processor_file_impl : public virtual file_impl, public virtual processor_file
{
public:
	processor_file_impl(std::string file_uri);
	processor_file_impl(file_impl &&);
	void collect_diags() const override;
	bool is_once_only() const override;

	//starts parser with new (empty) context
	parse_result parse(parse_lib_provider &);
	//starts parser with in the context of parameter
	parse_result parse(parse_lib_provider &, context::ctx_ptr);

	semantics::semantic_info & semantic_info();

	bool parse_info_updated() override;

	const std::set<std::string> & dependencies() override;

	virtual ~processor_file_impl() = default;
private:
	std::unique_ptr<analyzer> analyzer_;

	semantics::semantic_info sm_info_;
	bool parse_info_updated_ = false;

	std::set<std::string> dependencies_;
};

}
#endif // !HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_FILE_H
