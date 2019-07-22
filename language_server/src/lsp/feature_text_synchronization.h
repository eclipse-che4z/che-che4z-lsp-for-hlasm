#ifndef HLASMPLUGIN_LANGUAGESERVER_FEATURE_TEXTSYNCHRONIZATION_H
#define HLASMPLUGIN_LANGUAGESERVER_FEATURE_TEXTSYNCHRONIZATION_H

#include <vector>

#include "../feature.h"
#include "shared/workspace_manager.h"


namespace hlasm_plugin::language_server::lsp {

class feature_text_synchronization : public feature, public parser_library::highlighting_consumer
{
public:
	enum text_document_sync_kind {
		none = 0,
		full = 1,
		incremental = 2
	};

	feature_text_synchronization(parser_library::workspace_manager & ws_mngr, response_provider & response_provider);

	void register_methods(std::map<std::string, method> & methods) override;
	json virtual register_capabilities() override;
	void virtual initialize_feature(const json & initialise_params) override;

private:
	void on_did_open(const json & id, const json & params);
	void on_did_change(const json & id, const json & params);
	void on_did_close(const json & id, const json & params);

	// Inherited via highlighting_consumer
	virtual void consume_highlighting_info(parser_library::all_highlighting_info info) override;

};

}

#endif
