#ifndef HLASMPLUGIN_LANGUAGESERVER_TEST_SEND_MESSAGE_PROVIDER_MOCK_H
#define HLASMPLUGIN_LANGUAGESERVER_TEST_SEND_MESSAGE_PROVIDER_MOCK_H

#include "../src/server.h"

class send_message_provider_mock : public hlasm_plugin::language_server::send_message_provider
{
public:
	MOCK_METHOD1(reply, void(const json &));
};

#endif // !HLASMPLUGIN_LANGUAGESERVER_TEST_SEND_MESSAGE_PROVIDER_MOCK_H
