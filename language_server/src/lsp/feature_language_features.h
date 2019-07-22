#ifndef HLASMPLUGIN_LANGUAGESERVER_FEATURE_LANGUAGEFEATURES_H
#define HLASMPLUGIN_LANGUAGESERVER_FEATURE_LANGUAGEFEATURES_H

#include <vector>

#include "../feature.h"
#include "shared/workspace_manager.h"
#include "../logger.h"


namespace hlasm_plugin {
	namespace language_server {

		class feature_language_features : public feature
		{
		public:
			feature_language_features(parser_library::workspace_manager & ws_mngr, response_provider& response_provider);

			void virtual register_methods(std::map<std::string, method> & methods) override;
			json virtual register_capabilities() override;
			void virtual initialize_feature(const json & initialise_params) override;

		private:
			void definition(const json& id, const json & params);
			void references(const json& id, const json& params);
			void hover(const json& id, const json& params);
			void completion(const json& id, const json& params);
		};

	}
}

#endif
