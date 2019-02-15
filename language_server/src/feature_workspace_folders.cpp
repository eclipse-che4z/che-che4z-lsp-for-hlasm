#include <filesystem>

#include "feature.h"
#include "feature_workspace_folders.h"

namespace hlasm_plugin::language_server {

feature_workspace_folders::feature_workspace_folders(parser_library::workspace_manager & ws_mngr) :feature(ws_mngr) {}

void feature_workspace_folders::register_methods(std::map<std::string, method> &)
{

}

void feature_workspace_folders::register_notifications(std::map<std::string, notification> & notifications) 
{
	notifications.emplace("workspace/didChangeWorkspaceFolders",
		std::bind(&feature_workspace_folders::on_did_change_workspace_folders, this, std::placeholders::_1));
}

json feature_workspace_folders::register_capabilities()
{
	return json { { "workspace", Json
	{
		{"workspaceFolders", Json
		{
			{"supported", true},
			{"changeNotifications", true}
		}
		}
	} } };
}

void feature_workspace_folders::register_callbacks(response_callback response, response_error_callback error, notify_callback notify)
{
	callbacks_registered_ = true;
}

void feature_workspace_folders::initialize_feature(const json & initialize_params)
{
	bool ws_folders_support = false;
	auto capabs = initialize_params["capabilities"];
	auto ws = capabs.find("workspace");
	if (ws != capabs.end())
	{
		auto ws_folders = ws->find("workspaceFolders");
		if (ws_folders != ws->end())
			ws_folders_support = ws_folders->get<bool>();
	}

	if (ws_folders_support)
	{
		add_workspaces(initialize_params["workspaceFolders"]);
		return;
	}
	
	
	auto root_uri = initialize_params.find("rootUri");
	if (root_uri != initialize_params.end())
	{
		if (!root_uri->is_null())
		{
			std::string uri = root_uri->get<std::string>();
			add_workspace(uri, uri_to_path(uri));
			return;
		}
	}

	auto root_path = initialize_params.find("rootPath");
	if (root_path != initialize_params.end())
	{
		if (!root_path->is_null())
		{
			std::filesystem::path path(root_path->get<std::string>());
			add_workspace(path.lexically_normal().string(), path.lexically_normal().string());
		}
	}
}


void feature_workspace_folders::on_did_change_workspace_folders(const parameter & params)
{
	const auto & added = params["event"]["added"];
	const auto & removed = params["event"]["removed"];
	
	remove_workspaces(removed);
	add_workspaces(added);
}

void feature_workspace_folders::add_workspaces(const json & added)
{
	for (auto & it : added)
	{
		const std::string & name = it["name"].get<std::string>();
		std::string uri = it["uri"].get<std::string>();

		add_workspace(name, uri_to_path(uri));
	}
}
void feature_workspace_folders::remove_workspaces(const json & removed)
{
	for (auto it = removed.begin(); it != removed.end(); ++it)
	{
		std::string uri = (*it)["uri"].get<std::string>();

		ws_mngr_.remove_workspace(uri_to_path(uri).c_str());
	}
}
void feature_workspace_folders::add_workspace(const std::string & name, const std::string & path)
{
	ws_mngr_.add_workspace(name.c_str(), path.c_str());
}
}
