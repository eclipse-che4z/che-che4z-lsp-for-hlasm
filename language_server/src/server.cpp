
#include <functional>
#include <map>

#include "server.h"
#include "logger.h"

namespace hlasm_plugin {
namespace language_server {


enum TextDocumentSyncKind {
	None = 0,
	Full = 1,
	Incremental = 2
};



server::server()
{
	register_methods();
	register_notifications();
}



void server::register_methods()
{
	methods_.insert(std::make_pair("initialize", std::bind(&server::on_initialize, this, std::placeholders::_1, std::placeholders::_2)));
	methods_.insert(std::make_pair("shutdown", std::bind(&server::on_shutdown, this, std::placeholders::_1, std::placeholders::_2)));
}

void server::register_notifications()
{
	notifications_.insert(std::make_pair("exit", std::bind(&server::on_exit, this, std::placeholders::_1)));
}

void server::on_initialize(id id, parameter & param)
{
	client_initialize_params_ = param;

	Json json =
		Json{
			{  "capabilities",
			Json{
					{ "textDocumentSync", (int)TextDocumentSyncKind::Incremental },
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
					{ "definitionProvider", false },
					{ "documentHighlightProvider", false },
					{ "hoverProvider", false },
					{ "renameProvider", false },
					{ "documentSymbolProvider", false },
					{ "workspaceSymbolProvider", false },
				}
			} };

	reply_(id, json);

	//TODO initialization of parser library

	show_message("The capabilities of hlapv server were sent!", message_type::MT_INFO);
}

void server::on_shutdown(id id, parameter &)
{
	shutdown_request_received_ = true;

	//perform shutdown
	Json rep = Json{};
	reply_(id, rep);
}

void server::on_exit(parameter &)
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


void server::call_method(jsonrpcpp::request_ptr request)
{
	if (shutdown_request_received_)
		LOG_WARNING("Request " + request->method + " was received after shutdown request.");

	auto found = methods_.find(request->method);
	if (found != methods_.end())
	{
		(*found).second(request->id, request->params);
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
		(*found).second(request->params);
	}
	else
	{
		std::ostringstream ss;
		ss << "Notification " << request->method << " is not available on this server.";
		LOG_WARNING(ss.str());

	}
}


void server::register_callbacks(response_callback replyCb, notify_callback notifyCb, response_error_callback replyErrorCb)
{
	reply_ = replyCb;
	notify_ = notifyCb;
	replyError_ = replyErrorCb;
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
