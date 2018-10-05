#ifndef CONTEXT_LITERAL_STORAGE_H
#define CONTEXT_LITERAL_STORAGE_H

#include <unordered_set>
#include <list>
#include <string>
#include "common_types.h"

namespace hlasm_plugin{
namespace parser_library{
namespace context{


//storage for identifiers 
//changes strings of identifiers to indexes of this storage class for easier work with them
class id_storage
{
public:
	typedef std::string value_type;
	typedef std::string & reference;
	typedef const std::string & const_reference;
	typedef const std::string * const_pointer;
	typedef typename std::unordered_set<std::string>::size_type size_type;
	typedef typename std::unordered_set<std::string>::const_iterator const_iterator;

	size_type size() const { return lit_.size(); }	
	const_iterator begin() const { return lit_.begin(); }		
	const_iterator end() const { return lit_.end(); }		
	bool empty() const { return lit_.empty(); }

	const_pointer find(const std::string& val)
	{
		std::string up(val);
		to_upper(up);

		const_iterator tmp = lit_.find(up);

		if (tmp == lit_.end())
			return nullptr;

		return &*tmp;
	}

	const_pointer add(std::string value)
	{
		return &*lit_.insert(std::move(to_upper(value))).first;
	}

private:
	std::unordered_set<std::string> lit_;
};

using id_index = id_storage::const_pointer;

}
}
}


#endif
