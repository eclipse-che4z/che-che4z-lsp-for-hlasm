#ifndef HLASMPLUGIN_LANGUAGESERVER_FEATURE_WORKSPACEFOLDERS_H
#define HLASMPLUGIN_LANGUAGESERVER_FEATURE_WORKSPACEFOLDERS_H

#include <vector>

#include "feature.h"
#include "shared/workspace_manager.h"
#include "logger.h"


namespace hlasm_plugin {
namespace language_server {

class feature_workspace_folders : public feature
{
public:
	feature_workspace_folders(parser_library::workspace_manager & ws_mngr);

	void register_methods(std::map<std::string, method> &) override;
	void virtual register_notifications(std::map<std::string, notification> & notifications) override;
	json virtual register_capabilities() override;
	void virtual initialize_feature(const json & initialise_params) override;
private:
	void on_did_change_workspace_folders(const parameter & params);
	void add_workspaces(const json & added);
	void remove_workspaces(const json & removed);

	void add_workspace(const std::string & name, const std::string & uri);
};

}
}

#endif
