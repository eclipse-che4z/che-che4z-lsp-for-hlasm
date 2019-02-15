
#include <functional>
#include <map>
#include "server.h"
#include "logger.h"

#include "feature_workspace_folders.h"
#include "feature_text_synchronization.h"
#include "feature_language_features.h"

namespace hlasm_plugin {
namespace language_server {

server::server()
{
	features_.push_back(std::make_unique<feature_workspace_folders>(ws_mngr_));
	features_.push_back(std::make_unique<feature_text_synchronization>(ws_mngr_));
	features_.push_back(std::make_unique<feature_language_features>(ws_mngr_));
	register_methods();
	register_notifications();

	ws_mngr_.register_diagnostics_consumer(this);
}

void server::register_methods()
{
	methods_.insert(std::make_pair("initialize", std::bind(&server::on_initialize, this, std::placeholders::_1, std::placeholders::_2)));
	methods_.insert(std::make_pair("shutdown", std::bind(&server::on_shutdown, this, std::placeholders::_1, std::placeholders::_2)));

	for (auto & f : features_)
	{
		f->register_methods(methods_);
	}
}

void server::register_notifications()
{
	notifications_.insert(std::make_pair("exit", std::bind(&server::on_exit, this, std::placeholders::_1)));

	for (auto & f : features_)
	{
		f->register_notifications(notifications_);
	}
}

void server::on_initialize(id id, const parameter & param)
{
	client_initialize_params_ = param;

	//send server capabilities back
	json capabilities = json
	{
		{
			"capabilities", Json
			{
				{ "documentFormattingProvider", false },
				{ "documentRangeFormattingProvider", false },
				{ "codeActionProvider", false },
				{ "completionProvider",
					Json{
							{ "resolveProvider", false },
							{ "triggerCharacters",{ ".", ">", ":", "&" } }, //TODO
						}
				},
				{ "signatureHelpProvider",
					Json{
							{ "triggerCharacters",{ "(", "," } },
						}
				},
				{ "documentHighlightProvider", false },
				{ "hoverProvider", false },
				{ "renameProvider", false },
				{ "documentSymbolProvider", false },
				{ "workspaceSymbolProvider", false }
			}
		}
	};

	for (auto & f : features_)
	{
		json feature_cap = f->register_capabilities();
		capabilities["capabilities"].insert(feature_cap.begin(), feature_cap.end());
	}

	reply_(id, capabilities);


	for (auto & f : features_)
	{
		f->initialize_feature(param);
	}

	

	//show_message("The capabilities of hlasm language server were sent!", message_type::MT_INFO);
}

void server::on_shutdown(id id, const parameter &)
{
	shutdown_request_received_ = true;

	//perform shutdown
	Json rep = Json{};
	reply_(id, rep);
}

void server::on_exit(const parameter &)
{
	exit_notification_received_ = true;
}

void server::show_message(const std::string & message, message_type type)
{
	Json m{
		{ "type", (int)type },
		{ "message", message}
	};
	notify_("window/showMessage", m);
}

json diagnostic_related_info_to_json(parser_library::diagnostic & diag)
{
	json related;
	for (size_t i = 0; i < diag.related_info_size(); ++i)
	{
		related.push_back(json {
								{"location", feature::location_to_json(diag.related_info(i).location()) },
								{"message", diag.related_info(i).message()}
							});
	}
	return related;
}

void server::consume_diagnostics(parser_library::diagnostic_list diagnostics)
{
	std::map<std::string, std::vector<parser_library::diagnostic>> diags;
	
	for (size_t i = 0; i < diagnostics.diagnostics_size(); ++i)
	{
		auto d = diagnostics.diagnostics(i);
		diags[d.file_name()].push_back(d);
	}

	std::unordered_set<std::string> new_files;

	for (auto & file_diags : diags)
	{
		json diags_array = json::array();
		for (auto d : file_diags.second)
		{
			json one_json
			{
				{"range", feature::range_to_json(d.get_range())},
				{"code", d.code()},
				{"source", d.source()},
				{"message", d.message()},
				{"relatedInformation", diagnostic_related_info_to_json(d)}
			};
			if (d.severity() != parser_library::diagnostic_severity::unspecified)
			{
				one_json["severity"] = (int)d.severity();
				//one_json.insert(one_json.end(), { "severity", (int)d.severity() });
			}
			diags_array.push_back(std::move(one_json));
		}

		json publish_diags_params
		{
			{"uri", feature::path_to_uri(file_diags.first)},
			{"diagnostics", diags_array}
		};
		new_files.insert(file_diags.first);
		last_diagnostics_files.erase(file_diags.first);

		notify_("textDocument/publishDiagnostics", publish_diags_params);
	}

	for (auto & it : last_diagnostics_files)
	{
		json publish_diags_params
		{
			{"uri", feature::path_to_uri(it)},
			{"diagnostics", json::array()}
		};
		notify_("textDocument/publishDiagnostics", publish_diags_params);
	}

	last_diagnostics_files = std::move(new_files);
}



void server::call_method(jsonrpcpp::request_ptr request)
{
	if (shutdown_request_received_)
		LOG_WARNING("Request " + request->method + " was received after shutdown request.");

	auto found = methods_.find(request->method);
	if (found != methods_.end())
	{
		try
		{
			(*found).second(request->id, request->params.to_json());
		}
		catch (nlohmann::basic_json<>::exception e)
		{
			LOG_WARNING("There is an error regarding the JSON or LSP:" + std::string(e.what()));
		}
	}
	else
	{
		std::ostringstream ss;
		ss << "Method " << request->method << " is not available on this server.";
		LOG_WARNING(ss.str());
	}
}

void server::call_notification(jsonrpcpp::notification_ptr request)
{
	auto found = notifications_.find(request->method);
	if (found != notifications_.end())
	{
		try {
			found->second(request->params.to_json());
		}
		catch (nlohmann::basic_json<>::exception e)
		{
			LOG_WARNING("There is an error regarding the JSON or LSP:" + std::string(e.what()));
		}
	}
	else
	{
		std::ostringstream ss;
		ss << "Notification " << request->method << " is not available on this server.";
		LOG_WARNING(ss.str());
	}
}


void server::register_callbacks(response_callback reply, notify_callback notify, response_error_callback reply_error)
{
	reply_ = reply;
	notify_ = notify;
	replyError_ = reply_error;

	for (auto & f : features_)
	{
		f->register_callbacks(reply, reply_error, notify);
	}
}

bool server::is_exit_notification_received()
{
	return exit_notification_received_;
}

bool server::is_shutdown_request_received()
{
	return shutdown_request_received_;
}

} //namespace language_server
} //namespace hlasm_plugin
