
#include "feature.h"
#include "feature_language_features.h"

#include "shared/protocol.h"
namespace hlasm_plugin::language_server {

	feature_language_features::feature_language_features(parser_library::workspace_manager & ws_mngr) : feature(ws_mngr)
	{}

	void feature_language_features::register_methods(std::map<std::string, method> & methods)
	{
		methods.emplace("textDocument/definition",
			std::bind(&feature_language_features::definition, this, std::placeholders::_1, std::placeholders::_2));
		methods.emplace("textDocument/references",
			std::bind(&feature_language_features::references, this, std::placeholders::_1, std::placeholders::_2));
	}

	void feature_language_features::register_notifications(std::map<std::string, notification> &)
	{

	}

	json feature_language_features::register_capabilities()
	{
		return json
		{
			{ "definitionProvider", true},
			{ "referencesProvider", true}
		};
	}

	void feature_language_features::register_callbacks(response_callback response, response_error_callback error, notify_callback notify)
	{
		response_ = response;
		callbacks_registered_ = true;
	}

	void feature_language_features::initialize_feature(const json &)
	{
	}

	void feature_language_features::definition(id id, const parameter & params)
	{
		auto document_uri = params["textDocument"]["uri"].get<std::string>();
		auto pos = parser_library::position(params["position"]["line"].get<int>(), params["position"]["character"].get<int>());

		auto definition_position_uri = ws_mngr_.definition(uri_to_path(document_uri).c_str(), pos);
		document_uri = (definition_position_uri.uri()[0] == '\0') ? document_uri : path_to_uri(definition_position_uri.uri());
		json to_ret
		{
			{"uri", document_uri},
			{"range", range_to_json({definition_position_uri.pos(),definition_position_uri.pos()})
			}
		};
		response_(id,to_ret);
	}

	void feature_language_features::references(id id, const parameter & params)
	{
		auto document_uri = params["textDocument"]["uri"].get<std::string>();
		auto pos = parser_library::position(params["position"]["line"].get<int>(), params["position"]["character"].get<int>());
		json to_ret = json::array();
		auto references = ws_mngr_.references(uri_to_path(document_uri).c_str(), pos);
		for (size_t i = 0; i < references.size(); ++i)
		{
			auto ref = references.get_position_uri(i);
			to_ret.push_back(json
			{
				{ "uri", path_to_uri(ref.uri()) },
				{ "range",range_to_json({ ref.pos(), ref.pos() })
				}
			});
		}
		response_(id, to_ret);
	}
}
