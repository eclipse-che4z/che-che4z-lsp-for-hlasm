# Copyright (c) 2021 Broadcom.
# The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0
#
# Contributors:
#   Broadcom, Inc. - initial API and implementation

target_sources(library_test PRIVATE
	aread_test.cpp
	asm_instr_test.cpp
	ca_instr_test.cpp
	cics_preprocessor_test.cpp
	copy_test.cpp
	db2_preprocessor_test.cpp
	dc_test.cpp
	equ_test.cpp
	label_test.cpp
	loctr_test.cpp
	lookahead_test.cpp
	mach_instr_test.cpp
	occurence_collector_test.cpp
	opsyn_test.cpp
	org_test.cpp
	start_test.cpp
	end_instr_test.cpp
)

