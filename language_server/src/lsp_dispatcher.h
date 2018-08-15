#ifndef HLASMPLUGIN_HLASMLANGUAGESERVER_LSPDISPATCHER_H
#define HLASMPLUGIN_HLASMLANGUAGESERVER_LSPDISPATCHER_H

#include <iostream>

#include "server.h"

namespace HlasmPlugin {
namespace HlasmLanguageServer {

class LSPDispatcher
{
public:
	LSPDispatcher(std::ostream & out, Server & server);

	int runLanguageServerLoop(std::istream * in);
	static bool readMessage(std::istream * in, std::string & out);
	static bool readLine(std::istream * in, std::string &out);

	void writeMessage(const std::string & in);

	void reply(ID  id, Json & result);
	void replyError(ID id, Error & result);
	void notify(const std::string & method, Json & result);
private:
	Server & server;
	std::ostream & out;
};

}//namespace HlasmLanguageServer
}//namespace HlasmPlugin

#endif
