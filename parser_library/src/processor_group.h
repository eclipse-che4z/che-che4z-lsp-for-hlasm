#ifndef HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_GROUP_H
#define HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_GROUP_H

#include "library.h"
#include "diagnosable_impl.h"

namespace hlasm_plugin::parser_library {

//represents set of libraries
class processor_group : public diagnosable_impl
{
public:
	processor_group(const std::string & name) :name_(name) {}
	
	void collect_diags() const override
	{
		for (auto && lib : libs_)
		{
			collect_diags_from_child(*lib);
		}
	}

	void add_library(std::unique_ptr<library> library)
	{
		libs_.push_back(std::move(library));
	}

	const std::string & name() const
	{
		return name_;
	}

	const std::vector<std::unique_ptr<library> > & libraries() const
	{
		return libs_;
	}
private:
	std::vector<std::unique_ptr<library> > libs_;
	std::string name_;
};

}
#endif // !HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_GROUP_H
