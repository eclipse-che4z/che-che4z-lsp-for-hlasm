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

namespace HlasmPlugin {
namespace HlasmLanguageServer {

LSPDispatcher::LSPDispatcher(std::ostream & out, Server & server) : out(out), server(server)
{
	server.registerCallbacks(
		std::bind(&LSPDispatcher::reply, this, std::placeholders::_1, std::placeholders::_2),
		[&](const std::string & id, Json& notif) {this->notify(id, notif); },
		[&](ID id, Error& error) {this->replyError(id, error); });
}

void LSPDispatcher::writeMessage(const std::string & in)
{//copy of in is probably needed
	out << "Content-Length: " << in.size() << LSPENDLINE << LSPENDLINE;
	out << in;
}

void LSPDispatcher::reply(ID id, Json & result)
{
	if (!result.is_null())
	{
		jsonrpcpp::Response response{ id, result };
		writeMessage(response.to_json().dump());
	}
}

void LSPDispatcher::replyError(ID id, Error & error)
{
	jsonrpcpp::Response response{ id, error };
	writeMessage(response.to_json().dump());
}

void LSPDispatcher::notify(const std::string & method, Json & params)
{
	jsonrpcpp::Parameter p(params);
	jsonrpcpp::Notification message(method, p);
	writeMessage(message.to_json().dump());
}

bool LSPDispatcher::readLine(std::istream * in, std::string &out)
{

	static int bufSize = 64;

	size_t size = 0;
	out.clear();
	for (;;)
	{
		out.resize(size + bufSize);
		in->getline(&out[0], bufSize);

		size_t r = std::strlen(&out[size]);
		if (r > 0 && out[size + r - 1] == '\r') {
			out.resize(size + r - 1);
			return true;
		}
		size += r;
	}
}


bool LSPDispatcher::readMessage(std::istream * in, std::string & out)
{
	// A Language Server Protocol message starts with a set of HTTP headers,
	// delimited  by \r\n, and terminated by an empty line (\r\n).
	unsigned int ContentLength = 0;
	std::string line;
	int contentLen = std::string("Content-Length: ").size();

	while (true) {
		if (in->eof() || in->fail() || !readLine(in, line))
			return false;
		// We allow comments in headers. Technically this isn't part
		// of the LSP specification, but makes writing tests easier.
		if (line[0] == '#')
			continue;

		// Content-Length is a mandatory header, and the only one we handle.
		if (line.substr(0, contentLen) == "Content-Length: ")
		{
			if (ContentLength != 0)
			{
				LOG_WARNING("Duplicate Content-Length header received. The first one is ignored.");
			}

			std::stringstream str(line.substr(contentLen));

			str >> ContentLength;
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

	if (ContentLength > 1 << 30)
	{ // 1024M
		std::ostringstream ss;
		ss << "Refusing to read message with long Content-Length" << ContentLength << ". ";
		LOG_WARNING(ss.str());
		return false;
	}
	if (ContentLength == 0)
	{
		std::ostringstream ss;
		ss << "Warning: Missing Content-Length header, or zero-length message.";
		LOG_WARNING(ss.str());
		return false;
	}
	std::streamsize read;
	out.resize(ContentLength);
	for (std::streamsize pos = 0; pos < ContentLength; pos += read) {

		in->read(&out[0], ContentLength - pos);
		read = in->gcount();
		if (read == 0)
		{
			std::ostringstream ss;
			ss << "Input was aborted. Read only " << pos << " bytes of expected " << ContentLength;
			LOG_WARNING(ss.str());
			return false;
		}
	}

	return true;
}

int LSPDispatcher::runLanguageServerLoop(std::istream * in)
{
	jsonrpcpp::Parser parser;
	for (;;)
	{
		if (in->fail())
		{
			LOG_ERROR("IO error");
			return 1;
		}

		std::string message = "";

		
		if (readMessage(in, message))
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
					server.callMethod(request);

				}
				else if (entity->is_notification())
				{
					jsonrpcpp::notification_ptr notif = std::dynamic_pointer_cast<jsonrpcpp::Notification>(entity);
					server.callNotification(notif);
				}
			}
		}

		//if exit notifiaction came without prior shutdown request, return error 1
		if (server.getExitNotificationReceived())
		{
			if (server.getShutdownRequestReceived())
				return 0;
			else
				return 1;
		}
			



	}
}

} //namespace HlasmLanguageServer
} //namespace HlasmPlugin
