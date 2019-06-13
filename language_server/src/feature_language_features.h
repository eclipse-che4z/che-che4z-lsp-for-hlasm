#ifndef HLASMPLUGIN_LANGUAGESERVER_FEATURE_LANGUAGEFEATURES_H
#define HLASMPLUGIN_LANGUAGESERVER_FEATURE_LANGUAGEFEATURES_H

#include <vector>

#include "feature.h"
#include "shared/workspace_manager.h"
#include "logger.h"


namespace hlasm_plugin {
	namespace language_server {

		class feature_language_features : public feature
		{
		public:
			feature_language_features(parser_library::workspace_manager & ws_mngr);

			void virtual register_methods(std::map<std::string, method> & methods) override;
			void virtual register_notifications(std::map<std::string, notification> &) override;
			json virtual register_capabilities() override;
			void virtual register_callbacks(response_callback response, response_error_callback error, notify_callback notify) override;
			void virtual initialize_feature(const json & initialise_params) override;

		private:
			void definition(id id, const parameter & params);
			void references(id id, const parameter & params);
			void hover(id id, const parameter & params);
			void completion(id id, const parameter& params);

			response_callback response_;
		};

	}
}

#endif
