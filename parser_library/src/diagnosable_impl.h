#ifndef HLASMPLUGIN_PARSERLIBRARY_DIAGNOSABLE_IMPL_H
#define HLASMPLUGIN_PARSERLIBRARY_DIAGNOSABLE_IMPL_H

#include "diagnosable.h"

namespace hlasm_plugin::parser_library {

class diagnosable_impl : public virtual diagnosable
{
public:
	virtual diagnostic_container & diags() const override
	{
		return container;
	}

protected:

	virtual void collect_diags_from_child(const diagnosable & child) const
	{
		child.collect_diags();
		if (child.is_once_only())
		{
			container.insert(container.end(), std::make_move_iterator(child.diags().begin()),
				std::make_move_iterator(child.diags().end()));
			child.diags().clear();
		}
		else
		{
			container.insert(container.end(), child.diags().begin(), child.diags().end());
		}
	}

	virtual void add_diagnostic(diagnostic_s diagnostic) const override
	{
		container.push_back(std::move(diagnostic));
	}

	virtual bool is_once_only() const override
	{
		return true;
	}

	virtual ~diagnosable_impl() {};
private:
	mutable diagnostic_container container;
};

}


#endif