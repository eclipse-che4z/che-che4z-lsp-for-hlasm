#include "message_router.h"

hlasm_plugin::language_server::message_router::message_router(json_sink* optional_default_route)
    : default_route(optional_default_route)
{}

void hlasm_plugin::language_server::message_router::register_route(message_predicate predicate, json_sink& sink)
{
    routes.emplace_back(std::move(predicate), &sink);
}

void hlasm_plugin::language_server::message_router::write(const nlohmann::json& msg)
{
    for (const auto& route : routes)
    {
        if (route.first(msg))
        {
            route.second->write(msg);
            return;
        }
    }
    if (default_route)
        default_route->write(msg);
}

void hlasm_plugin::language_server::message_router::write(nlohmann::json&& msg)
{
    for (const auto& route : routes)
    {
        if (route.first(msg))
        {
            route.second->write(std::move(msg));
            return;
        }
    }
    if (default_route)
        default_route->write(std::move(msg));
}
