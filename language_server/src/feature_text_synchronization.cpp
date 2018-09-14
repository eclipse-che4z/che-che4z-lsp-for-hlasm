
#include "feature.h"
#include "feature_text_synchronization.h"

#include "shared/protocol.h"
namespace hlasm_plugin::language_server {

feature_text_synchronization::feature_text_synchronization(parser_library::workspace_manager & ws_mngr) :feature(ws_mngr) {}

void feature_text_synchronization::register_methods(std::map<std::string, method> &)
{

}

void feature_text_synchronization::register_notifications(std::map<std::string, notification> & notifications)
{
	notifications.emplace("textDocument/didOpen",
		std::bind(&feature_text_synchronization::on_did_open, this, std::placeholders::_1));
	notifications.emplace("textDocument/didChange",
		std::bind(&feature_text_synchronization::on_did_change, this, std::placeholders::_1));
	notifications.emplace("textDocument/didClose",
		std::bind(&feature_text_synchronization::on_did_close, this, std::placeholders::_1));
}

json feature_text_synchronization::register_capabilities()
{
	//there is no reason why not ask for notifications (most of them is
	//ignored anyway).
	//we cant process willSaveWaitUntil because it is a request and we dont
	//want many hanging requests
	return json 
	{
		{ "textDocumentSync", Json
		{
			{"openClose", true},
			{"change", (int)text_document_sync_kind::incremental},
			{"willSave", true},
			{"willSaveWaitUntil", false},
			{"save", Json
			{
				{"includeText", true}
			}
			}
	} } };
}

void feature_text_synchronization::initialize_feature(const json &)
{
}

void feature_text_synchronization::on_did_open(const parameter & params)
{
	json text_doc = params["textDocument"];
	std::string doc_uri = text_doc["uri"].get<std::string>();
	auto version = text_doc["version"].get<nlohmann::json::number_unsigned_t>();
	std::string text = text_doc["text"].get<std::string>();
	
	ws_mngr_.did_open_file(uri_to_path(doc_uri).c_str(), version, text.c_str(), text.size());
}

void feature_text_synchronization::on_did_change(const parameter & params)
{
	json text_doc = params["textDocument"];
	std::string doc_uri = text_doc["uri"].get<std::string>();
	
	auto version = text_doc["version"].get<nlohmann::json::number_unsigned_t>();

	json content_changes = params["contentChanges"];

	std::vector<parser_library::document_change> changes;
	std::vector<std::string> texts(content_changes.size());
	size_t i = 0;
	for (auto & ch : content_changes)
	{
		texts[i] = ch["text"].get<std::string>();

		auto range_it = ch.find("range");
		if (range_it == ch.end())
		{
			changes.emplace_back(texts[i].c_str(), texts[i].size());
		}
		else
		{
			changes.emplace_back(parse_range(ch["range"]), texts[i].c_str(), texts[i].size());
		}

		++i;
	}
	ws_mngr_.did_change_file(uri_to_path(doc_uri).c_str(), version, &*changes.begin(), changes.size());
}

void feature_text_synchronization::on_did_close(const parameter & params)
{
	std::string uri = params["textDocument"]["uri"].get<std::string>();
	
	ws_mngr_.did_close_file(uri_to_path(uri).c_str());
}

}
