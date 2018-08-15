#ifndef HLASMPLUGIN_HLASMLANGUAGESERVER_SERVER_H
#define HLASMPLUGIN_HLASMLANGUAGESERVER_SERVER_H

#include <functional>

#include "jsonrp.hpp"
#include "json.hpp"
namespace hlasm_plugin {
namespace language_server {

enum class message_type {
	MT_ERROR = 1,
	MT_WARNING = 2,
	MT_INFO = 3,
	MT_LOG = 4
};

using id = jsonrpcpp::Id;
using error = jsonrpcpp::Error;
//                     void reply(ID id, Json result, Json error)
// mby take string instead of json? the json is deserialized in the next step anyway
using response_callback = std::function<void(id, Json&)>;
using response_error_callback = std::function<void(id, error&)>;
using notify_callback = std::function<void(const std::string &, Json&)>;

using parameter = jsonrpcpp::Parameter;

class server
{
	using method = std::function<void(id, parameter&)>;
	using notification = std::function<void(parameter&)>;

public:
	server();

	void call_method(jsonrpcpp::request_ptr request);
	void call_notification(jsonrpcpp::notification_ptr notification);

	void register_callbacks(response_callback replyCb, notify_callback notifyCb, response_error_callback replyErrorCb);

	bool is_shutdown_request_received();
	bool is_exit_notification_received();

private:
	response_callback reply_;
	notify_callback notify_;
	response_error_callback replyError_;

	bool shutdown_request_received_ = false;
	bool exit_notification_received_ = false;

	std::map<std::string, method> methods_;
	std::map<std::string, notification> notifications_;

	parameter client_initialize_params_;

	

	void register_methods();
	void register_notifications();

	//requests
	void on_initialize(id id, parameter & param);
	void on_shutdown(id id, parameter & param);

	

	//notifications
	void on_exit(parameter & param);

	//client notifications
	void show_message(const std::string & message, message_type type);
};

}//namespace language_server
}//namespace hlasm_plugin


#endif
