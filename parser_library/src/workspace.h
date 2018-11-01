#ifndef HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_H
#define HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_H

#include <string>
#include <filesystem>
#include <unordered_set>
#include <memory>
#include <vector>

#include "file_manager.h"
#include "../src/generated/parser_library_export.h"
#include "library.h"

namespace hlasm_plugin {
namespace parser_library {

using ws_uri = std::string;
using proc_grp_id = std::string;
using program_id = std::string;

//represents pair program => processor group - saves
//information that a program uses certain processor group
struct program
{
	program(program_id prog_id, proc_grp_id pgroup) : prog_id(prog_id), pgroup(pgroup) {}
	program_id prog_id;
	proc_grp_id pgroup;
};

//represents set of libraries
struct processor_group
{
	processor_group(const std::string & name) :name(name) {}
	std::string name;
	std::vector<std::unique_ptr<library> > libs;
};

class workspace
{
public:
	
	workspace(ws_uri uri, file_manager & file_manager);
	workspace(ws_uri uri, std::string name, file_manager & file_manager);
	
	workspace(const workspace & ws) = delete;
	workspace & operator= (const workspace &) = delete;

	workspace(workspace && ws) = default;
	workspace & operator= (workspace &&) = default;

	const processor_group & get_proc_grp(const proc_grp_id & proc_grp) const;
	const processor_group & get_proc_grp_by_program(const std::string & program) const;

	void open();
	void close();
private:

	std::string name_;
	ws_uri uri_;
	file_manager & file_manager_;
	std::unordered_map<std::string, file *> files_;
	std::unordered_map<proc_grp_id, processor_group> proc_grps_;
	std::vector<program> pgm_conf_;

	processor_group implicit_proc_grp;

	bool is_hlasm_ws_ = false;
	bool opened_ = false;

	void load_config();
};

}
}

#endif // !HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_H
