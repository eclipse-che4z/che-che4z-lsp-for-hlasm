#ifndef HLASMPLUGIN_HLASMLANGUAGESERVER_SERVER_H
#define HLASMPLUGIN_HLASMLANGUAGESERVER_SERVER_H

#include <functional>

#include "jsonrp.hpp"
#include "json.hpp"
namespace HlasmPlugin {
namespace HlasmLanguageServer {

enum MessageType {
	MTError = 1,
	MTWarning = 2,
	MTInfo = 3,
	MTLog = 4
};

using ID = jsonrpcpp::Id;
using Error = jsonrpcpp::Error;
//                     void reply(ID id, Json result, Json error)
// mby take string instead of json? the json is deserialized in the next step anyway
using ResponseCallback = std::function<void(ID, Json&)>;
using ResponseErrorCallback = std::function<void(ID, Error&)>;
using NotifyCallback = std::function<void(const std::string &, Json&)>;

using Parameter = jsonrpcpp::Parameter;

class Server
{
	using Method = std::function<void(ID, Parameter&)>;
	using Notification = std::function<void(Parameter&)>;

public:
	Server();

	void callMethod(jsonrpcpp::request_ptr request);
	void callNotification(jsonrpcpp::notification_ptr notification);

	void registerCallbacks(ResponseCallback replyCb, NotifyCallback notifyCb, ResponseErrorCallback replyErrorCb);

	bool getShutdownRequestReceived();
	bool getExitNotificationReceived();

private:
	ResponseCallback reply;
	NotifyCallback notify;
	ResponseErrorCallback replyError;

	bool shutdownRequestReceived = false;
	bool exitNotificationReceived = false;

	std::map<std::string, Method> Methods;
	std::map<std::string, Notification> Notifications;

	Parameter clientInitializeParams;

	

	void registerMethods();
	void registerNotifications();

	//requests
	void onInitialize(ID id, Parameter & param);
	void onShutdown(ID id, Parameter & param);

	

	//notifications
	void onExit(Parameter & param);

	//client notifications
	void showMessage(const std::string & message, MessageType type);
};

}//namespace HlasmLanguageServer
}//namespace HlasmPlugin


#endif