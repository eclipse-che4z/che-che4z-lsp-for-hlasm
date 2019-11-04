#ifndef HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_IMPL_H
#define HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_IMPL_H

#include "shared/workspace_manager.h"
#include "workspace.h"
#include "file_manager_impl.h"
#include "debugging/debugger.h"
#include "debugging/debug_lib_provider.h"

namespace hlasm_plugin::parser_library
{

class workspace_manager::impl : public diagnosable_impl, public debugging::debug_event_consumer_s
{
public:
	impl(std::atomic<bool>* cancel = nullptr) : file_manager_(cancel), implicit_workspace_({ file_manager_ }), cancel_(cancel) {}
	impl(const impl &) = delete;
	impl & operator= (const impl &) = delete;

	impl(impl &&) = default;
	impl & operator= (impl &&) = default;

	size_t get_workspaces(ws_id * workspaces, size_t max_size)
	{
		size_t size = 0;
		
		for (auto it = workspaces_.begin(); size < max_size && it != workspaces_.end(); ++size, ++it)
		{
			workspaces[size] = &it->second;
		}
		return size;
	}

	size_t get_workspaces_count()
	{
		return workspaces_.size();
	}

	void add_workspace(std::string name, std::string uri)
	{
		auto ws = workspaces_.emplace(name, workspace { uri, name, file_manager_ });
		ws.first->second.open();

		notify_diagnostics_consumers();
	}
	void remove_workspace(std::string uri)
	{
		auto it = workspaces_.find(uri);
		if (it == workspaces_.end())
			return; //erase does no action, if the key does not exist
		workspaces_.erase(uri);
		notify_diagnostics_consumers();
	}
	
	void did_open_file(const std::string & document_uri, version_t version, std::string text)
	{
		file_manager_.did_open_file(document_uri, version, std::move(text));
		if (cancel_ && *cancel_) return;

		workspace & ws = ws_path_match(document_uri);
		ws.did_open_file(document_uri);
		if (cancel_ && *cancel_) return;

		notify_highlighting_consumers();
		notify_diagnostics_consumers();
		// only on open
		notify_performance_consumers(document_uri);
	}
	void did_change_file(const std::string document_uri, version_t version, const document_change * changes, size_t ch_size)
	{
		file_manager_.did_change_file(document_uri, version, changes, ch_size);
		if (cancel_ && *cancel_) return;

		workspace & ws = ws_path_match(document_uri);
		ws.did_change_file(document_uri, changes, ch_size);
		if (cancel_ && *cancel_) return;

		notify_highlighting_consumers();
		notify_diagnostics_consumers();
	}

	void did_close_file(const std::string document_uri)
	{
		workspace& ws = ws_path_match(document_uri);
		ws.did_close_file(document_uri);
		notify_highlighting_consumers();
		notify_diagnostics_consumers();
	}

	void did_change_watched_files(std::vector<std::string> paths)
	{
		for (const auto& path : paths)
		{
			workspace& ws = ws_path_match(path);
			ws.did_change_watched_files(path);
		}
		notify_highlighting_consumers();
		notify_diagnostics_consumers();
	}

	void register_highlighting_consumer(highlighting_consumer * consumer)
	{
		hl_consumers_.push_back(consumer);
	}

	void register_diagnostics_consumer(diagnostics_consumer * consumer)
	{
		diag_consumers_.push_back(consumer);
	}

	void register_performance_metrics_consumer(performance_metrics_consumer* consumer)
	{
		metrics_consumers_.push_back(consumer);
	}

	semantics::position_uri_s found_position;
	position_uri definition(std::string document_uri, const position pos)
	{
		auto file = file_manager_.find(document_uri);
		if (dynamic_cast<processor_file*>(file.get()) != nullptr)
			found_position = file_manager_.find_processor_file(document_uri)->get_lsp_info().go_to_definition(pos);
		else
			found_position = { document_uri,pos };
		return found_position;
	}

	std::vector<semantics::position_uri_s> found_refs;
	position_uris references(std::string document_uri, const position pos)
	{
		auto file = file_manager_.find(document_uri);
		if (dynamic_cast<processor_file*>(file.get()) != nullptr)
			found_refs = file_manager_.find_processor_file(document_uri)->get_lsp_info().references(pos);
		else
			found_refs.clear();
		return { found_refs.data(), found_refs.size() };
	}

	std::vector<std::string> output;
	std::vector<const char*> coutput;
	const string_array hover(const char * document_uri, const position pos)
	{
		auto file = file_manager_.find(document_uri);
		if (dynamic_cast<processor_file*>(file.get()) != nullptr)
			output = file_manager_.find_processor_file(document_uri)->get_lsp_info().hover(pos);
		else
			output.clear();
		coutput.clear();
		for (const auto & str : output)
			coutput.push_back(str.c_str());
		
		return { coutput.data(), coutput.size() };
	}

	semantics::completion_list_s completion_result;
	completion_list completion(const char* document_uri, const position pos, const char trigger_char, int trigger_kind)
	{
		auto file = file_manager_.find(document_uri);
		if (dynamic_cast<processor_file*>(file.get()) != nullptr)
			completion_result = file_manager_.find_processor_file(document_uri)->get_lsp_info().completion(pos, trigger_char, trigger_kind);
		else
			completion_result = semantics::completion_list_s();
		return completion_result;
	}

	void launch(std::string file_name, bool stop_on_entry)
	{
		workspace & ws = ws_path_match(file_name);
		processor_file_ptr file = file_manager_.add_processor_file(file_name);
		debugger_ = std::make_unique<debugging::debugger>(*this, debug_cfg_);
		debug_lib_provider_ = std::make_unique<debugging::debug_lib_provider>(ws);
		debugger_->launch(file, *debug_lib_provider_, stop_on_entry);
	}

	void register_debug_event_consumer(debug_event_consumer & consumer)
	{
		debug_event_consumers_.insert(&consumer);
	}

	void unregister_debug_event_consumer(debug_event_consumer & consumer)
	{
		debug_event_consumers_.erase(&consumer);
	}

	void next()
	{
		if (!debugger_)
			return;

		debugger_->next();
	}

	void step_in()
	{
		if (!debugger_)
			return;

		debugger_->step_in();
	}

	void disconnect()
	{
		if (!debugger_)
			return;

		debugger_->disconnect();

		debugger_ = nullptr;
	}
	
	stack_frames get_stack_frames()
	{
		if (!debugger_)
			return {nullptr, 0};
		
		auto & res = debugger_->stack_frames();

		return { res.data(), res.size() };
	}

	scopes get_scopes(frame_id_t frame_id)
	{
		if (!debugger_)
			return { nullptr, 0 };

		auto & res = debugger_->scopes(frame_id);

		return { res.data(), res.size() };
	}

	
	variables get_variables(var_reference_t var_reference)
	{
		if (!debugger_)
			return { nullptr, 0 };
		
		auto & res = debugger_->variables(var_reference);
		temp_variables_.resize(res.size());
		for (size_t i = 0; i < res.size(); ++i)
			temp_variables_[i] = res[i].get();
		
		return { temp_variables_.data(), temp_variables_.size() };
	}

	void set_breakpoints(std::string source_path, std::vector<breakpoint> brs)
	{
		debug_cfg_.set_breakpoints(debugging::breakpoints{ debugging::source(std::move(source_path)), std::move(brs) });
	}

	void continue_debug()
	{
		if (!debugger_)
			return;

		debugger_->continue_debug();
	}

private:
	debugging::debug_config debug_cfg_;
	std::unique_ptr<debugging::debugger> debugger_;
	std::set<debug_event_consumer *> debug_event_consumers_;
	std::unique_ptr<debugging::debug_lib_provider> debug_lib_provider_;

	// Inherited via debug_event_consumer_s
	virtual void stopped(const std::string & reason, const std::string & addtl_info) override
	{
		for (auto c : debug_event_consumers_)
		{
			c->stopped(reason.c_str(), addtl_info.c_str());
		}
	}

	virtual void exited(int exit_code)
	{
		for (auto c : debug_event_consumers_)
		{
			c->exited(exit_code);
			c->terminated();
		}
	}

	virtual void collect_diags() const override
	{
		collect_diags_from_child(file_manager_);

		for (auto & it : workspaces_)
			collect_diags_from_child(it.second);
	}

	void notify_highlighting_consumers()
	{
		auto file_list = file_manager_.list_updated_files();
		all_highlighting_info hl_info(file_list.data(), file_list.size());
		for (auto consumer : hl_consumers_)
		{
			consumer->consume_highlighting_info(hl_info);
		}
	}

	void notify_diagnostics_consumers()
	{
		diags().clear();
		collect_diags();
		diagnostic_list l(diags().data(), diags().size());
		for (auto consumer : diag_consumers_)
		{
			consumer->consume_diagnostics(l);
		}
	}

	void notify_performance_consumers(const std::string& document_uri)
	{
		auto file = file_manager_.find(document_uri);
		auto proc_file = dynamic_cast<processor_file*>(file.get());
		if (proc_file)
		{
			auto metrics = proc_file->get_metrics();
			for (auto consumer : metrics_consumers_)
			{
				consumer->consume_performance_metrics(metrics);
			}
		}
	}

	size_t prefix_match(const std::string & first, const std::string & second)
	{
		size_t match = 0;
		size_t i1 = 0;
		size_t i2 = 0;
		while (first[i1] == second[i2] && i1 < first.size() && i2 < second.size())
		{
			++i1;
			++i2;
			++match;
		}
		return match;
	}

	//returns implicit workspace, if the file does not belong to any workspace
	workspace & ws_path_match(const std::string & document_uri)
	{
		size_t max = 0;
		workspace * max_ws = nullptr;
		for (auto & ws : workspaces_)
		{
			size_t match = prefix_match(document_uri, ws.second.uri());
			if (match > max && match >= ws.first.size())
			{
				max = match;
				max_ws = &ws.second;
			}
		}
		if (max_ws == nullptr)
			return implicit_workspace_;
		else
			return *max_ws;
	}
	std::vector<debugging::variable *> temp_variables_;

	std::unordered_map<std::string, workspace> workspaces_;
	file_manager_impl file_manager_;
	workspace implicit_workspace_;
	std::atomic<bool>* cancel_;

	std::vector<highlighting_consumer *> hl_consumers_;
	std::vector<diagnostics_consumer *> diag_consumers_;
	std::vector<performance_metrics_consumer*> metrics_consumers_;
};
}

#endif // !HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_IMPL_H
