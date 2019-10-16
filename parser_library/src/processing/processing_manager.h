#ifndef PROCESSING_PROCESSING_MANAGER_H
#define PROCESSING_PROCESSING_MANAGER_H

#include "../parse_lib_provider.h"
#include "processing_state_listener.h"
#include "opencode_provider.h"
#include "branching_provider.h"
#include "attribute_provider.h"
#include "statement_fields_parser.h"

#include <stack>

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//main class for processing of the opencode
//is constructed with base statement provider and has stack of statement processors which take statements from providers and go through the code creating other providers and processors
//it holds those providers and processors and manages the whole processing
class processing_manager : public processing_state_listener, public branching_provider, public attribute_provider, public diagnosable_ctx
{
public:
	processing_manager(
		std::unique_ptr<opencode_provider> base_provider, 
		context::hlasm_context& hlasm_ctx, 
		const library_data data,
		parse_lib_provider& lib_provider,
		statement_fields_parser& parser);

	//method that starts the processing loop
	void start_processing(std::atomic<bool>* cancel);

	virtual void collect_diags() const override;
private:
	context::hlasm_context& hlasm_ctx_;
	parse_lib_provider& lib_provider_;

	std::vector<processor_ptr> procs_;
	std::vector<provider_ptr> provs_;

	opencode_provider& opencode_prov_;

	statement_provider& find_provider();
	void finish_processor();

	virtual void start_macro_definition(macrodef_start_data start) override;
	virtual void finish_macro_definition(macrodef_processing_result result) override;
	virtual void start_lookahead(lookahead_start_data start) override;
	virtual void finish_lookahead(lookahead_processing_result result) override;
	virtual void start_copy_member(copy_start_data start) override;
	virtual void finish_copy_member(copy_processing_result result) override;

	virtual void jump_in_statements(context::id_index target, range symbol_range) override;
	virtual void register_sequence_symbol(context::id_index target, range symbol_range) override;
	std::unique_ptr<context::opencode_sequence_symbol> create_opencode_sequence_symbol(context::id_index name, range symbol_range);

	virtual attribute_provider::resolved_reference_storage 
		lookup_forward_attribute_references(attribute_provider::forward_reference_storage references) override;
		
	void perform_opencode_jump(context::source_position statement_position, context::source_snapshot snapshot);
};

}
}
}
#endif
