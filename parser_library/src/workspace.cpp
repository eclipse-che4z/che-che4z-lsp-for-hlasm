#include <string>
#include <filesystem>
#include <string>
#include <regex>

#include "workspace.h"
#include "json.hpp"
#include "parser_tools.h"
#include "processor.h"

namespace hlasm_plugin {
namespace parser_library {

workspace::workspace(ws_uri uri, std::string name, file_manager & file_manager)
	: name_(name), uri_(uri), file_manager_(file_manager), implicit_proc_grp("pg_implicit"), ws_path_(uri)
{
	proc_grps_path_ = ws_path_ / FILENAME_PROC_GRPS;
	pgm_conf_path_ = ws_path_ / FILENAME_PGM_CONF;
}

workspace::workspace(ws_uri uri, file_manager & file_manager) : workspace(uri, uri, file_manager)
{
}

workspace::workspace(file_manager & file_manager) : workspace("", file_manager)
{
	opened_ = true;
}

void workspace::collect_diags() const
{
	for (auto & pg : proc_grps_)
	{
		collect_diags_from_child(pg.second);
	}

	for (auto & diag : config_diags_)
		add_diagnostic(diag);
}

void workspace::add_proc_grp(processor_group pg)
{
	proc_grps_.emplace(pg.name(), std::move(pg));
}

bool program_id_match(const std::string & filename, const program_id & program)
{
	//TODO: actual wildcard matching instead of regex
	std::regex prg_regex(program);
	return std::regex_match(filename, prg_regex);
}

const processor_group & workspace::get_proc_grp_by_program(const std::string & filename) const
{
	assert(opened_);
	for (const auto & pgm : pgm_conf_)
	{
		std::filesystem::path fname_path(filename);
		
		if(program_id_match(fname_path.lexically_relative(uri_).lexically_normal().string(), pgm.prog_id))
			return proc_grps_.at(pgm.pgroup);
	}
	return implicit_proc_grp;
}

const ws_uri & workspace::uri()
{
	return uri_;
}

void workspace::parse_file(const std::string & file_uri)
{
	std::filesystem::path file_path(file_uri);
	//add support for hlasm to vscode (auto detection??) and do the decision based on languageid
	if (file_path == proc_grps_path_ || file_path == pgm_conf_path_)
	{
		if (load_config())
		{
			for (auto f : dependants_)
				f->parse(*this);
			for (auto f : dependants_)
				filter_and_close_dependencies_(f->files_to_close(), f);
		}

		return;
	}
	//what about removing files??? what if depentands_ points to not existing file?
	std::vector<processor_file *> files_to_parse;

	for (auto f : dependants_)
	{
		for (auto & name : f->dependencies())
		{
			if (name == file_uri)
			{
				files_to_parse.push_back(f);
				break;
			}
		}
	}

	if (files_to_parse.empty())
	{
		auto f = file_manager_.find_processor_file(file_uri);
		if (f)
			files_to_parse.push_back(f);
	}

	for (auto f : files_to_parse)
	{
		f->parse(*this);
		if (!f->dependencies().empty())
			dependants_.insert(f);
	}

	//second check after all dependants are there to close all files that used to be dependencies
	for (auto f : files_to_parse)
		filter_and_close_dependencies_(f->files_to_close(), f);
}

void workspace::refresh_libraries()
{
	for (auto& proc_grp : proc_grps_)
	{
		for (auto& lib : proc_grp.second.libraries())
		{
			lib->refresh();
		}
	}
}

void workspace::did_open_file(const std::string & file_uri)
{
	parse_file(file_uri);
}

void workspace::did_close_file(const std::string& file_uri)
{
	//first check whether the file is a dependency
	//if so, simply close it, no other action is needed
	if (is_dependency_(file_uri))
	{
		file_manager_.did_close_file(file_uri);
		return;
	}

	// find if the file is a dependant
	auto file = std::find_if(dependants_.begin(), dependants_.end(), [&file_uri](processor_file * dep) {return dep->get_file_name() == file_uri; });
	if (file != dependants_.end())
	{
		//filter the dependencies that should not be closed
		filter_and_close_dependencies_((*file)->dependencies(), *file);
		// remove it from dependants
		dependants_.erase(file);
	}

	//close the file itself
	file_manager_.did_close_file(file_uri);
	file_manager_.remove_file(file_uri);
}

void workspace::did_change_file(const std::string file_uri, const document_change *, size_t)
{
	parse_file(file_uri);
}

void workspace::did_change_watched_files(const std::string& file_uri)
{
	refresh_libraries();
	parse_file(file_uri);
}

void hlasm_plugin::parser_library::workspace::open()
{
	load_config();
}

void hlasm_plugin::parser_library::workspace::close()
{
	opened_ = false;
}

const processor_group & workspace::get_proc_grp(const proc_grp_id & proc_grp) const
{
	assert(opened_);
	return proc_grps_.at(proc_grp);
}

//open config files and parse them
bool hlasm_plugin::parser_library::workspace::load_config()
{
	config_diags_.clear();

	opened_ = true;

	std::filesystem::path ws_path(uri_);
	using json = nlohmann::json;

	//proc_grps.json parse
	file * proc_grps_file = file_manager_.add_file((ws_path / FILENAME_PROC_GRPS).string());
	
	if (proc_grps_file->update_and_get_bad())
		return false;

	json proc_grps_json;
	try
	{
		proc_grps_json = nlohmann::json::parse(proc_grps_file->get_text());
		proc_grps_.clear();
	}
	catch(nlohmann::json::exception e)
	{
		//could not load proc_grps
		config_diags_.push_back(diagnostic_s::error_W002(proc_grps_file->get_file_name(), name_));
		return false;
	}

	json pgs = proc_grps_json["pgroups"];

	for (auto & pg : pgs)
	{
		const std::string & name = pg["name"].get<std::string>();
		const json & libs = pg["libs"];

		processor_group prc_grp(name);

		for (auto & lib_path_json : libs)
		{
			if (lib_path_json.is_string())
			{
				//the added '/' will ensure the path always ends with directory separator
				const std::string p = lib_path_json.get<std::string>();
				std::filesystem::path lib_path(p.empty() ? p : p + '/');
				if(lib_path.is_absolute())
					prc_grp.add_library(std::make_unique<library_local>(file_manager_, lib_path.lexically_normal().string()));
				else if (lib_path.is_relative())
					prc_grp.add_library(std::make_unique<library_local>(file_manager_, (ws_path / lib_path).lexically_normal().string()));
				//else ignore, publish warning
			}
		}

		add_proc_grp(std::move(prc_grp));
	}


	//pgm_conf.json parse
	file * pgm_conf_file = file_manager_.add_file((ws_path / FILENAME_PGM_CONF).string());

	if (pgm_conf_file->update_and_get_bad())
		return false;

	json pgm_conf_json;

	try
	{
		pgm_conf_json = nlohmann::json::parse(pgm_conf_file->get_text());
		pgm_conf_.clear();
	}
	catch (nlohmann::json::exception e)
	{
		config_diags_.push_back(diagnostic_s::error_W003(pgm_conf_file->get_file_name(), name_));
		return false;
	}
	json pgms = pgm_conf_json["pgms"];

	for (auto & pgm : pgms)
	{
		const std::string & program = pgm["program"].get<std::string>();
		const std::string & pgroup = pgm["pgroup"].get<std::string>();

		if (proc_grps_.find(pgroup) != proc_grps_.end())
		{
			pgm_conf_.emplace_back(std::move(program), std::move(pgroup));
		}
		else
		{ 
			config_diags_.push_back(diagnostic_s::error_W004(pgm_conf_file->get_file_name(), name_));
		}
	}
	return true;
}

void workspace::filter_and_close_dependencies_(const std::set<std::string>& dependencies, processor_file * file)
{
	std::set<std::string> filtered;
	 // filters out externally open files
	for (auto& dependency : dependencies)
	{
		auto dependency_file = file_manager_.find_processor_file(dependency);
		if (dependency_file && !dependency_file->get_lsp_editing())
			filtered.insert(dependency);
	}

	// filters the files that are dependencies of other dependants and externally open files
	for (auto dependant : dependants_)
	{
		for (auto& dependency : dependant->dependencies())
		{
			if (dependant->get_file_name() != file->get_file_name() && filtered.find(dependency) != filtered.end())
				filtered.erase(dependency);
		}
	}

	// close all exclusive dependencies of file
	for (auto& dep : filtered)
	{
		file_manager_.did_close_file(dep);
		file_manager_.remove_file(dep);
	}
}

bool workspace::is_dependency_(const std::string & file_uri)
{
	for (auto dependant : dependants_)
	{
		for (auto& dependency : dependant->dependencies())
		{
			if (dependency == file_uri)
				return true;
		}
	}
	return false;
}

parse_result workspace::parse_library(const std::string & library, context::hlasm_context& hlasm_ctx, const library_data data)
{
	auto & proc_grp = get_proc_grp_by_program(hlasm_ctx.opencode_file_name());
	for (auto && lib : proc_grp.libraries())
	{
		processor * found = lib->find_file(library);
		if (found)
			return found->parse(*this, hlasm_ctx, data);
	}
	
	return false;
}


}
}
