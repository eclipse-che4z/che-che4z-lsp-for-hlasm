#ifndef HLASMPLUGIN_PARSERLIBRARY_MESSAGE_CONSUMER_H
#define HLASMPLUGIN_PARSERLIBRARY_MESSAGE_CONSUMER_H

#include <string>

#include "parser_library_export.h"

namespace hlasm_plugin::parser_library {

enum class message_type
{
    MT_ERROR = 1,
    MT_WARNING = 2,
    MT_INFO = 3,
    MT_LOG = 4
};

class PARSER_LIBRARY_EXPORT message_consumer
{
public:
    virtual void show_message(const std::string& message, message_type type) = 0;
    virtual ~message_consumer() {};
};

} // namespace hlasm_plugin::parser_library

#endif
