
#include <functional>
#include <map>
#include <sstream>
#include <string>

#include "server.h"
#include "logger.h"

namespace hlasm_plugin::language_server {

server::server(parser_library::workspace_manager & ws_mngr) : ws_mngr_(ws_mngr)
{
}

void server::register_methods()
{
	for (auto & f : features_)
	{
		f->register_methods(methods_);
	}
}

void server::call_method(const std::string & method, const json & id, const json & args)
{
	if (shutdown_request_received_)
		LOG_WARNING("Request " + method + " was received after shutdown request.");

	auto found = methods_.find(method);
	if (found != methods_.end())
	{
		try
		{
			(*found).second(id, args);
		}
		catch (nlohmann::basic_json<>::exception e)
		{
			LOG_WARNING("There is an error regarding the JSON or LSP:" + std::string(e.what()));
		}
	}
	else
	{
		std::ostringstream ss;
		ss << "Method " << method << " is not available on this server.";
		LOG_WARNING(ss.str());
	}
}

bool server::is_exit_notification_received()
{
	return exit_notification_received_;
}

void server::set_send_message_provider(send_message_provider* provider)
{
	send_message_ = provider;
}

bool server::is_shutdown_request_received()
{
	return shutdown_request_received_;
}

} //namespace hlasm_plugin::language_server::lsp
