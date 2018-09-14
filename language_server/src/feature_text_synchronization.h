#ifndef HLASMPLUGIN_LANGUAGESERVER_FEATURE_TEXTSYNCHRONIZATION_H
#define HLASMPLUGIN_LANGUAGESERVER_FEATURE_TEXTSYNCHRONIZATION_H

#include <vector>

#include "feature.h"
#include "shared/workspace_manager.h"
#include "logger.h"


namespace hlasm_plugin {
namespace language_server {

class feature_text_synchronization : public feature
{
public:
	enum text_document_sync_kind {
		none = 0,
		full = 1,
		incremental = 2
	};

	feature_text_synchronization(parser_library::workspace_manager & ws_mngr);

	void register_methods(std::map<std::string, method> &) override;
	void virtual register_notifications(std::map<std::string, notification> & notifications) override;
	json virtual register_capabilities() override;
	void virtual initialize_feature(const json & initialise_params) override;
private:
	void on_did_open(const parameter & params);
	void on_did_change(const parameter & params);
	void on_did_close(const parameter & params);
};

}
}

#endif
