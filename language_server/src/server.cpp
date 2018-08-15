
#include <functional>
#include <map>

#include "server.h"
#include "logger.h"

namespace HlasmPlugin {
namespace HlasmLanguageServer {


enum TextDocumentSyncKind {
	None = 0,
	Full = 1,
	Incremental = 2
};



Server::Server()
{
	registerMethods();
	registerNotifications();
}



void Server::registerMethods()
{
	Methods.insert(std::make_pair("initialize", std::bind(&Server::onInitialize, this, std::placeholders::_1, std::placeholders::_2)));
	Methods.insert(std::make_pair("shutdown", std::bind(&Server::onShutdown, this, std::placeholders::_1, std::placeholders::_2)));
}

void Server::registerNotifications()
{
	Notifications.insert(std::make_pair("exit", std::bind(&Server::onExit, this, std::placeholders::_1)));
}

void Server::onInitialize(ID id, Parameter & param)
{
	clientInitializeParams = param;

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

	reply(id, json);

	//TODO initialization of parser library

	showMessage("The capabilities of hlapv server were sent!", MessageType::MTInfo);
}

void Server::onShutdown(ID id, Parameter &)
{
	shutdownRequestReceived = true;

	//perform shutdown
	Json rep = Json{};
	reply(id, rep);
}

void Server::onExit(Parameter &)
{
	exitNotificationReceived = true;
}

void Server::showMessage(const std::string & message, MessageType type)
{
	Json m{
		{ "type", (int)type },
		{ "message", message}
	};
	notify("window/showMessage", m);
}


void Server::callMethod(jsonrpcpp::request_ptr request)
{
	if (shutdownRequestReceived)
		LOG_WARNING("Request " + request->method + " was received after shutdown request.");

	auto found = Methods.find(request->method);
	if (found != Methods.end())
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

void Server::callNotification(jsonrpcpp::notification_ptr request)
{
	auto found = Notifications.find(request->method);
	if (found != Notifications.end())
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


void Server::registerCallbacks(ResponseCallback replyCb, NotifyCallback notifyCb, ResponseErrorCallback replyErrorCb)
{
	reply = replyCb;
	notify = notifyCb;
	replyError = replyErrorCb;
}

bool Server::getExitNotificationReceived()
{
	return exitNotificationReceived;
}

bool Server::getShutdownRequestReceived()
{
	return shutdownRequestReceived;
}

} //namespace HlasmLanguageServer
} //namespace HlasmPlugin
