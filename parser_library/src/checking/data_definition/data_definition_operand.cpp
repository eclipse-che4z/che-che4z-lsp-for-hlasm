#include "data_definition_operand.h"

using namespace hlasm_plugin::parser_library::checking;
using namespace hlasm_plugin::parser_library;

const data_def_type* data_definition_operand::access_data_def_type() const
{
	return data_def_type::access_data_def_type(type.value, extension.value);
}

bool data_definition_operand::check_type_and_extension(const diagnostic_collector& add_diagnostic) const
{
	auto found = data_def_type::types_extensions.find(type.value);
	if (found == data_def_type::types_extensions.end())
	{
		add_diagnostic(diagnostic_op::error_D012(type.rng));
		return false;
	}
	if (extension.present && found->second.find(extension.value) == found->second.end())
	{
		add_diagnostic(diagnostic_op::error_D013(extension.rng, std::string(1, type.value)));
		return false;
	}
	return true;
}

uint64_t data_definition_operand::get_length() const
{
	auto def_type = access_data_def_type();
	if (def_type)
		return def_type->get_length(*this);
	else
		return 0;
}

context::alignment data_definition_operand::get_alignment() const
{
	auto def_type = access_data_def_type();
	if (def_type)
		return def_type->get_alignment(length.present);
	else
		return context::no_align;
}