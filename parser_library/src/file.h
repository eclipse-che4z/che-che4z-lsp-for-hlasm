/*
 * Copyright (c) 2019 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Broadcom, Inc. - initial API and implementation
 */

#ifndef HLASMPLUGIN_PARSERLIBRARY_FILE_H
#define HLASMPLUGIN_PARSERLIBRARY_FILE_H

#include <string>
#include <istream>
#include <filesystem>

#include "shared/protocol.h"
#include "diagnosable.h"

namespace hlasm_plugin {
namespace parser_library {

using file_uri = std::string;

class file : public virtual diagnosable
{
public:
	virtual const file_uri & get_file_name() = 0;
	virtual const std::string & get_text() = 0;
	virtual bool update_and_get_bad() = 0;
	virtual bool get_lsp_editing() = 0;

	virtual version_t get_version() = 0;

	virtual void did_open(std::string new_text, version_t version) = 0;
	virtual void did_change(std::string new_text) = 0;
	virtual void did_change(range range, std::string new_text) = 0;
	virtual void did_close() = 0;
};


}
}

#endif
