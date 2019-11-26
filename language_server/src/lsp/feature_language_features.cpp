/*
 * Copyright (c) 2019 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Broadcom, Inc. - initial API and implementation
 */


#include "../feature.h"
#include "feature_language_features.h"

#include "shared/protocol.h"

#include <iostream>

namespace hlasm_plugin::language_server {

	feature_language_features::feature_language_features(parser_library::workspace_manager & ws_mngr, response_provider& response_provider) : feature(ws_mngr, response_provider)
	{}

	void feature_language_features::register_methods(std::map<std::string, method> & methods)
	{
		methods.emplace("textDocument/definition",
			std::bind(&feature_language_features::definition, this, std::placeholders::_1, std::placeholders::_2));
		methods.emplace("textDocument/references",
			std::bind(&feature_language_features::references, this, std::placeholders::_1, std::placeholders::_2));
		methods.emplace("textDocument/hover",
			std::bind(&feature_language_features::hover, this, std::placeholders::_1, std::placeholders::_2));
		methods.emplace("textDocument/completion",
			std::bind(&feature_language_features::completion, this, std::placeholders::_1, std::placeholders::_2));

	}

	json feature_language_features::register_capabilities()
	{
		return json
		{
			{ "definitionProvider", true},
			{ "referencesProvider", true},
			{ "hoverProvider", true},
			{ "completionProvider",
				{
					{"resolveProvider", false},
					{"triggerCharacters",{"&","."}}
				}
			}
		};
	}

	void feature_language_features::initialize_feature(const json &)
	{
	}

	void feature_language_features::definition(const json & id, const json& params)
	{
		auto document_uri = params["textDocument"]["uri"].get<std::string>();
		auto pos = parser_library::position(params["position"]["line"].get<int>(), params["position"]["character"].get<int>());

		auto definition_position_uri = ws_mngr_.definition(uri_to_path(document_uri).c_str(), pos);
		document_uri = (definition_position_uri.uri()[0] == '\0') ? document_uri : path_to_uri(definition_position_uri.uri());
		json to_ret
		{
			{"uri", document_uri},
			{"range", range_to_json({definition_position_uri.pos(),definition_position_uri.pos()})}
		};
		response_->respond(id,"",to_ret);
	}

	void feature_language_features::references(const json& id, const json& params)
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
				{ "range",range_to_json({ ref.pos(), ref.pos() })}
			});
		}
		response_->respond(id, "", to_ret);
	}
	void feature_language_features::hover(const json& id, const json& params)
	{
		auto document_uri = params["textDocument"]["uri"].get<std::string>();
		auto pos = parser_library::position(params["position"]["line"].get<int>(), params["position"]["character"].get<int>());

		json hover_arr = json::array();
		auto hover_list = ws_mngr_.hover(uri_to_path(document_uri).c_str(), pos);
		for (size_t i = 0; i < hover_list.size; i++)
		{
			hover_arr.push_back(hover_list.arr[i]);
		}
		response_->respond(id, "", json{ { "contents", hover_arr } });
	}
	void feature_language_features::completion(const json& id, const json& params)
	{
		auto document_uri = params["textDocument"]["uri"].get<std::string>();
		auto pos = parser_library::position(params["position"]["line"].get<int>(), params["position"]["character"].get<int>());
		//no trigger character
		char trigger_char = '\0';
		int trigger_kind = params["context"]["triggerKind"].get<int>();
		if (trigger_kind == 2)
			trigger_char = params["context"]["triggerCharacter"].get<std::string>()[0];
		auto completion_list = ws_mngr_.completion(uri_to_path(document_uri).c_str(), pos,trigger_char,trigger_kind);
		json to_ret = json::value_t::null;
		json completion_item_array = json::array();
		for (size_t i = 0; i < completion_list.count(); i++)
		{
			auto item = completion_list.item(i);
			completion_item_array.push_back(json
				{
					{"label",item.label()},
					{"kind",item.kind()},
					{"detail",item.detail()},
					{"documentation",item.documentation()},
					{"deprecated",item.deprecated()},
					{"insertText",item.insert_text()}
				});
		}
		to_ret = json
		{
			{"isIncomplete", completion_list.is_incomplete()},
			{"items", completion_item_array}
		};

		response_->respond(id, "", to_ret);
	}
}
