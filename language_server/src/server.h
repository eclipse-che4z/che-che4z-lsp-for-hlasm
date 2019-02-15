#ifndef HLASMPLUGIN_HLASMLANGUAGESERVER_SERVER_H
#define HLASMPLUGIN_HLASMLANGUAGESERVER_SERVER_H

#include <functional>
#include <memory>
#include <unordered_set>

#include "jsonrp.hpp"
#include "json.hpp"

#include "shared/workspace_manager.h"
#include "common_types.h"
#include "feature.h"


namespace hlasm_plugin {
namespace language_server {

enum class message_type {
	MT_ERROR = 1,
	MT_WARNING = 2,
	MT_INFO = 3,
	MT_LOG = 4
};

class server : public parser_library::diagnostics_consumer
{

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

	std::vector<std::unique_ptr<feature> > features_;

	std::map<std::string, method> methods_;
	std::map<std::string, notification> notifications_;

	parameter client_initialize_params_;

	parser_library::workspace_manager ws_mngr_;

	void register_methods();
	void register_notifications();

	//requests
	void on_initialize(id id, const parameter & param);
	void on_shutdown(id id, const parameter & param);

	std::unordered_set<std::string> last_diagnostics_files;
	

	//notifications
	void on_exit(const parameter & param);

	//client notifications
	void show_message(const std::string & message, message_type type);

	virtual void consume_diagnostics(parser_library::diagnostic_list diagnostics) override;
	
};

}//namespace language_server
}//namespace hlasm_plugin


#endif
