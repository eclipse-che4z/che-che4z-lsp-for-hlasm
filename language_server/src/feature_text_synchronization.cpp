
#include "feature.h"
#include "feature_text_synchronization.h"

#include "shared/protocol.h"
namespace hlasm_plugin::language_server {

feature_text_synchronization::feature_text_synchronization(parser_library::workspace_manager & ws_mngr) : feature(ws_mngr)
{
	ws_mngr.register_highlighting_consumer(this);
}

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

void feature_text_synchronization::register_callbacks(response_callback response, response_error_callback error, notify_callback notify)
{
	notify_ = notify;
	callbacks_registered_ = true;
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

void feature_text_synchronization::consume_highlighting_info(parser_library::all_semantic_info info)
{
	auto f = info.files();
	for (size_t i = 0; i < info.files_count(); i++)
	{
		auto fi = info.file_info(f[i]);

		std::string scope;
		json tokens_array = json::array();
		for (size_t j = 0; j < fi.token_count(); j++)
		{
			switch (fi.token(j).scope)
			{
			case parser_library::semantics::scopes::label:
				scope = "label";
				break;
			case parser_library::semantics::scopes::instruction:
				scope = "instruction";
				break;
			case parser_library::semantics::scopes::remark:
				scope = "remark";
				break;
			case parser_library::semantics::scopes::comment:
				scope = "comment";
				break;
			case parser_library::semantics::scopes::ignored:
				scope = "ignored";
				break;
			case parser_library::semantics::scopes::continuation:
				scope = "continuation";
				break;
			case parser_library::semantics::scopes::operator_symbol:
				scope = "operator";
				break;
			case parser_library::semantics::scopes::seq_symbol:
				scope = "seqSymbol";
				break;
			case parser_library::semantics::scopes::var_symbol:
				scope = "varSymbol";
				break;
			case parser_library::semantics::scopes::string:
				scope = "string";
				break;
			case parser_library::semantics::scopes::number:
				scope = "number";
				break;
			case parser_library::semantics::scopes::operand:
				scope = "operand";
				break;
			}
			tokens_array.push_back(json{
				{ "lineStart",fi.token(j).token_range.start.line },
				{ "columnStart",fi.token(j).token_range.start.column },
				{ "lineEnd",fi.token(j).token_range.end.line },
				{ "columnEnd",fi.token(j).token_range.end.column },
				{ "scope", scope }
				});
		}

		json continuations_array = json::array();
		continuations_array.push_back(json{fi.continuation_column(), fi.continue_column()});
		for (size_t j = 0; j < fi.continuation_count(); j++)
		{
			continuations_array.push_back(json{
				fi.continuation(j).line, fi.continuation(j).column
			});
		}

		json args = json { 
		{ "textDocument",
			json { 
				{ "uri", feature::path_to_uri(fi.document_uri()) },
				{ "version",fi.document_version() }
		} },
		{ "tokens", tokens_array },
		{ "continuationPositions", continuations_array}
		};

		if (callbacks_registered_)
			notify_("textDocument/semanticHighlighting", args);
	}
}
}
