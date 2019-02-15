#include "lsp_dispatcher.h"
#include "logger.h"
#include "jsonrp.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <memory>

#ifdef _WIN32
#define LSPENDLINE "\n"
#else
#define LSPENDLINE "\r\n"
#endif

namespace hlasm_plugin {
namespace language_server {

lsp_dispatcher::lsp_dispatcher(std::ostream & out, server & server) : out_(out), server_(server)
{
	server.register_callbacks(
		std::bind(&lsp_dispatcher::reply, this, std::placeholders::_1, std::placeholders::_2),
		[&](const std::string & id, const Json& notif) {this->notify(id, notif); },
		[&](id id, const error& error) {this->reply_error(id, error); });
}

void lsp_dispatcher::write_message(const std::string & in)
{//copy of in is probably needed
	LOG_INFO(in);
	out_ << "Content-Length: " << in.size() << LSPENDLINE << LSPENDLINE;
	out_ << in;
}

void lsp_dispatcher::reply(id id, const  Json & result)
{
	if (!result.is_null())
	{
		jsonrpcpp::Response response{ id, result };
		write_message(response.to_json().dump());
	}
}

void lsp_dispatcher::reply_error(id id, const error & error)
{
	jsonrpcpp::Response response{ id, error };
	write_message(response.to_json().dump());
}

void lsp_dispatcher::notify(const std::string & method, const Json & params)
{
	jsonrpcpp::Parameter p(params);
	jsonrpcpp::Notification message(method, p);
	write_message(message.to_json().dump());
}

bool lsp_dispatcher::read_line(std::istream & in, std::string &out)
{

	static int bufSize = 64;

	size_t size = 0;
	out.clear();
	for (;;)
	{
		out.resize(size + bufSize);
		in.getline(&out[0], bufSize);

		if (!in.good())
			return false;
		size_t r = std::strlen(&out[size]);
		if (r > 0 && out[size + r - 1] == '\r') {
			out.resize(size + r - 1);
			return true;
		}
		size += r;
	}
}


bool lsp_dispatcher::read_message(std::istream & in, std::string & out)
{
	// A Language Server Protocol message starts with a set of HTTP headers,
	// delimited  by \r\n, and terminated by an empty line (\r\n).
	unsigned int content_length = 0;
	std::string line;
	size_t content_len = std::string("Content-Length: ").size();

	while (true) {
		if (!in.good() || !read_line(in, line))
			return false;
		// We allow comments in headers. Technically this isn't part
		// of the LSP specification, but makes writing tests easier.
		if (line[0] == '#')
			continue;

		// Content-Length is a mandatory header, and the only one we handle.
		if (line.substr(0, content_len) == "Content-Length: ")
		{
			if (content_length != 0)
			{
				LOG_WARNING("Duplicate Content-Length header received. The first one is ignored.");
			}

			std::stringstream str(line.substr(content_len));

			str >> content_length;
			continue;
		}
		else if (line.empty())
		{
			// An empty line indicates the end of headers.
			// Go ahead and read the JSON.
			break;
		}
		else
		{
			// It's another header, ignore it.
		}
	}

	if (content_length > 1 << 30)
	{ // 1024M
		std::ostringstream ss;
		ss << "Refusing to read message with long Content-Length" << content_length << ". ";
		LOG_WARNING(ss.str());
		return false;
	}
	if (content_length == 0)
	{
		std::ostringstream ss;
		ss << "Warning: Missing Content-Length header, or zero-length message.";
		LOG_WARNING(ss.str());
		return false;
	}
	std::streamsize read;
	out.resize(content_length);
	for (std::streamsize pos = 0; pos < content_length; pos += read) {

		in.read(&out[0], content_length - pos);
		read = in.gcount();
		if (read == 0)
		{
			std::ostringstream ss;
			ss << "Input was aborted. Read only " << pos << " bytes of expected " << content_length;
			LOG_WARNING(ss.str());
			return false;
		}
	}

	return true;
}

int lsp_dispatcher::run_server_loop(std::istream & in)
{
	jsonrpcpp::Parser parser;
	for (;;)
	{
		if (in.eof())
		{
			LOG_INFO("End of File");
			return 0;
		}

		if (in.fail() || in.bad())
		{
			LOG_ERROR("IO error");
			return 1;
		}

		std::string message = "";

		
		if (read_message(in, message))
		{

			LOG_INFO(message);

			jsonrpcpp::entity_ptr entity = 0;
			try {
				entity = parser.parse(message);
			}
			catch (jsonrpcpp::ParseErrorException)
			{
				LOG_WARNING("Could not parse received JSON: " + message);
			}

			if (entity)
			{
				if (entity->is_request())
				{
					jsonrpcpp::request_ptr request = std::dynamic_pointer_cast<jsonrpcpp::Request>(entity);
					server_.call_method(request);

				}
				else if (entity->is_notification())
				{
					jsonrpcpp::notification_ptr notif = std::dynamic_pointer_cast<jsonrpcpp::Notification>(entity);
					server_.call_notification(notif);
				}
			}
		}

		//if exit notifiaction came without prior shutdown request, return error 1
		if (server_.is_exit_notification_received())
		{
			if (server_.is_shutdown_request_received())
				return 0;
			else
				return 1;
		}
	}
}

} //namespace language_server
} //namespace hlasm_plugin
