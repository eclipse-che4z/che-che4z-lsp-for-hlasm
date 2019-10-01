#include <iostream>
#include <string>
#include <sstream>
#include <memory>

#include "dispatcher.h"
#include "logger.h"
#include "json.hpp"

namespace hlasm_plugin {
namespace language_server {

constexpr const char* didOpen = "textDocument/didOpen";
constexpr const char* didChange = "textDocument/didChange";

dispatcher::dispatcher(std::istream& in, std::ostream& out, server& server, std::atomic<bool> * cancel) :
	server_(server), in_(in), out_(out), worker_(&dispatcher::handle_request_,this), cancel_(cancel)
{
	server_.set_send_message_provider(this);
}

static const std::string content_length_string_ = "Content-Length: ";

void dispatcher::write_message(const std::string & in)
{
	LOG_INFO(in);
	std::lock_guard<std::mutex> guard(mtx_);
	out_.write(content_length_string_.c_str(), content_length_string_.size());
	std::string size = std::to_string(in.size());
	out_.write(size.c_str(), size.size());
	out_.write("\r\n\r\n", 4);
	out_.write(in.c_str(), in.size());
	out_.flush();
}

void dispatcher::reply(const json & message)
{
	// do not respond if the request was cancelled
	if (cancel_ != nullptr  && *cancel_)
		return;
	write_message(message.dump());
}

void dispatcher::handle_request_()
{
	// endless cycle in separate thread, pick up work if there is some, otherwise wait for work
	while (true)
	{
		std::unique_lock<std::mutex> lock(q_mtx_);
		//wait for work to come
		if (requests_.empty())
			cond_.wait(lock, [&] { return !requests_.empty(); });
		//get first request
		auto to_run = std::move(requests_.front());
		requests_.pop_front();
		//remember file name that is about to be parsed
		currently_running_file_ = get_request_file_(to_run.message);
		// if the request is valid, do not cancel
		// if not, cancel the parsing right away, only the file manager should update the data
		if (cancel_)
			*cancel_ = !to_run.valid;
		//unlock the mutex, main thread may add new requests
		lock.unlock();
		//handle the request
		server_.message_received(to_run.message);
	}
}

std::string dispatcher::get_request_file_(json r)
{
	auto method = r["method"].get<std::string>();
	if (method == didOpen || method == didChange)
		return r["params"]["textDocument"]["uri"].get<std::string>();
	return std::string();
}

bool dispatcher::read_message(std::string & out)
{
	// A Language Server Protocol message starts with a set of HTTP headers,
	// delimited  by \r\n, and terminated by an empty line (\r\n).
	std::streamsize content_length = 0;
	std::string line;
	size_t content_len = std::string(content_length_string_).size();
	for(;;)
	{
		if (in_.eof() || in_.fail())
			return false;

		in_ >> line;

		// Content-Length is a mandatory header, and the only one we handle.
		if (line.substr(0, content_len) == content_length_string_)
		{
			if (content_length != 0)
			{
				LOG_WARNING("Duplicate Content-Length header received. The first one is ignored.");
			}

			std::stringstream str(line.substr(content_len));

			str >> content_length;
			continue;
		}
		else if (line == "\r")
		{
			// An empty line indicates the end of headers.
			// Go ahead and read the JSON.

			// The >> function left \n as the next character, we need to remove it
			// before we use in_.read(). When we use in_ >> line, all whitespace is
			// ignored, so it is only needed in this case
			in_.ignore();

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

	std::streamsize pos = 0;

	std::streamsize read;
	out.resize((size_t)content_length);
	for (; pos < content_length; pos += read) {

		in_.read(&out[(size_t)pos], content_length - pos);
		read = in_.gcount();
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

int dispatcher::run_server_loop()
{
	std::string message;
	for (;;)
	{
		if (in_.fail())
		{
			LOG_ERROR("IO error");
			return 1;
		}
		if (in_.eof())
		{
			LOG_ERROR("IO: unexpected end of file");
			return 1;
		}
		
		message.clear();

		if (read_message(message))
		{
			if(message.size() < 500)
				LOG_INFO(message);

			json message_json = 0;
			try {
				message_json = nlohmann::json::parse(message);
			}
			catch (const nlohmann::json::exception &)
			{
				LOG_WARNING("Could not parse received JSON: " + message);
				continue;
			}

			//add request to q
			{
				std::unique_lock<std::mutex> lock(q_mtx_);
				//get new file
				auto file = get_request_file_(message_json);
				// if the new file is the same as the currently running one, cancel the old one
				if (currently_running_file_ == file && currently_running_file_ != "")
					*cancel_ = true;
				// mark redundant requests as non valid
				for (auto & req : requests_)
				{
					if (get_request_file_(req.message) == file)
						req.valid = false;
				}
				//finally add it to the q
				requests_.push_back({ message_json, true });
			}
			//wake up the worker thread
			cond_.notify_one();
		}

		//if exit notification came without prior shutdown request, return error 1
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
