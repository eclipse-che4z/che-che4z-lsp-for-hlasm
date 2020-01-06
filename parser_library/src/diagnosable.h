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

#ifndef HLASMPARSER_PARSERLIBRARY_DIAGNOSTICABLE_H
#define HLASMPARSER_PARSERLIBRARY_DIAGNOSTICABLE_H

#include <string>
#include <vector>

#include "shared/protocol.h"
#include "diagnostic.h"

namespace hlasm_plugin::parser_library {

template <typename T>
class collectable
{
public:
	using diagnostic_container = std::vector<T>;

	virtual void collect_diags() const = 0;
	virtual diagnostic_container & diags() const = 0;
	virtual void add_diagnostic(T diagnostic) const = 0;
	virtual bool is_once_only() const = 0;

	virtual ~collectable() = 0;
};

template <typename T>
inline collectable<T>::~collectable() {};

using diagnosable = collectable<diagnostic_s>;

}

#endif

