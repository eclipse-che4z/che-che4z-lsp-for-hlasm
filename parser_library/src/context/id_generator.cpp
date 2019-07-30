#include "id_generator.h"

using namespace hlasm_plugin::parser_library::context;

id_generator::id_generator(id_storage& storage) : storage_(storage), last_id(0) {}

id_index id_generator::get_id()
{
	if (!released_.empty())
	{
		auto tmp = released_.back();
		released_.pop_back();
		return tmp;
	}
	else
	{
		auto new_id_str = " " + std::to_string(last_id);
		++last_id;
		return storage_.add(new_id_str);
	}
}

void id_generator::release_id(id_index id)
{
	released_.push_back(id);
}

bool id_generator::is_generated(id_index id)
{
	return !id->empty() && (*id)[0] == ' ';
}
