#ifndef HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_VARIABLE_H
#define HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_VARIABLE_H

#include <memory>
#include <optional>

#include "../context/variable.h"
#include "shared/protocol.h"

namespace hlasm_plugin::parser_library::debugging
{
class variable;

using variable_ptr = std::unique_ptr<variable>;

class variable
{
public:
	const std::string& get_value() const;
	const std::string& get_name() const;

	virtual set_type type() const = 0;

	virtual bool is_scalar() const = 0;

	virtual std::vector<variable_ptr> values() const = 0;
	virtual size_t size() const = 0;

	var_reference_t var_reference = 0;

	virtual ~variable() = default;
protected:

	virtual const std::string& get_string_value() const = 0;
	virtual const std::string& get_string_name() const = 0;

	std::optional<std::string> name_;
	std::optional<std::string> value_;
};

}

#endif // !HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_VARIABLE_H
