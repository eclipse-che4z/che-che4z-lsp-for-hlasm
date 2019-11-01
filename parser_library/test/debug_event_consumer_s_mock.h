#ifndef HLASMPLUGIN_PARSERLIBRARY_TEST_DEBUG_EVENT_CONSUMER_S_MOCK_H
#define HLASMPLUGIN_PARSERLIBRARY_TEST_DEBUG_EVENT_CONSUMER_S_MOCK_H

#include "gmock/gmock.h"

#include "../src/debugging/debugger.h"

using namespace hlasm_plugin::parser_library::debugging;

class debug_event_consumer_s_mock : public debug_event_consumer_s
{
	volatile std::atomic<bool> stopped_ = false;
	std::atomic<bool> exited_ = false;
	

public:
	std::atomic<size_t> stop_count = 0;

	virtual void stopped(const std::string& reason, const std::string& addtl_info) override
	{
		++stop_count;
		while (stopped_);
		stopped_ = true;
	}

	virtual void exited(int exit_code)
	{
		exited_ = true;
	}

	
	void wait_for_stopped()
	{
		while (!stopped_);
		stopped_ = false;
	}

	void wait_for_exited()
	{
		while (!exited_);
		stopped_ = false;
	}
};

#endif // !HLASMPLUGIN_PARSERLIBRARY_TEST_DEBUG_EVENT_CONSUMER_S_MOCK_H
