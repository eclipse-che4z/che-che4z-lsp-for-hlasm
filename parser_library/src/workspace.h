#ifndef HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_H
#define HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_H

#include <string>
#include <filesystem>
#include <unordered_set>
#include <memory>
#include <vector>

#include "file_manager.h"
#include "library.h"
#include "diagnosable_impl.h"
#include "processor_group.h"
#include "processor.h"

namespace hlasm_plugin {
namespace parser_library {

using ws_uri = std::string;
using proc_grp_id = std::string;
using program_id = std::string;
using ws_highlight_info = std::unordered_map< std::string, semantics::highlighting_info >;

//represents pair program => processor group - saves
//information that a program uses certain processor group
struct program
{
	program(program_id prog_id, proc_grp_id pgroup) : prog_id(prog_id), pgroup(pgroup) {}
	program_id prog_id;
	proc_grp_id pgroup;
};



class workspace : public diagnosable_impl, public parse_lib_provider
{
public:
	//just a dummy workspace with no libraries
	workspace(file_manager & file_manager);
	workspace(ws_uri uri, file_manager & file_manager);
	workspace(ws_uri uri, std::string name, file_manager & file_manager);
	
	workspace(const workspace & ws) = delete;
	workspace & operator= (const workspace &) = delete;

	workspace(workspace && ws) = default;
	workspace & operator= (workspace &&) = default;

	void collect_diags() const override;

	void add_proc_grp(processor_group pg);
	const processor_group & get_proc_grp(const proc_grp_id & proc_grp) const;
	const processor_group & get_proc_grp_by_program(const std::string & program) const;

	void parse_file(const std::string& file_uri);
	void refresh_libraries();
	void did_open_file(const std::string & file_uri);
	void did_close_file(const std::string& file_uri);
	void did_change_file(const std::string document_uri, const document_change * changes, size_t ch_size);
	void did_change_watched_files(const std::string& file_uri);

	virtual parse_result parse_library(const std::string & library, context::hlasm_context& hlasm_ctx,const library_data data) override;
	virtual bool has_library(const std::string & library, context::hlasm_context& hlasm_ctx) const override;

	const ws_uri & uri();

	void open();
	void close();
protected:

	file_manager& get_file_manager();

private:
	constexpr static char FILENAME_PROC_GRPS[] = "proc_grps.json";
	constexpr static char FILENAME_PGM_CONF[] = "pgm_conf.json";
	constexpr static char HLASM_PLUGIN_FOLDER[] = ".hlasmplugin";

	std::string name_;
	ws_uri uri_;
	file_manager & file_manager_;

	std::unordered_map<proc_grp_id, processor_group> proc_grps_;
	std::vector<program> pgm_conf_;

	processor_group implicit_proc_grp;

	std::filesystem::path ws_path_;
	std::filesystem::path proc_grps_path_;
	std::filesystem::path pgm_conf_path_;

	bool is_hlasm_ws_ = false;
	bool opened_ = false;

	bool load_config();
	
	//files, that depend on others (e.g. open code files that use macros)
	std::set<std::string> dependants_;

	diagnostic_container config_diags_;

	void filter_and_close_dependencies_(const std::set<std::string>& dependencies, processor_file_ptr file);
	bool is_dependency_(const std::string & file_uri);
};

}
}

#endif // !HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_H
