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

Project(fuzzer)

if(BUILD_FUZZER)
	add_executable(library_fuzzer fuzzer.cpp
	)
    target_link_libraries(library_fuzzer parser_library)


    set_target_properties(library_fuzzer PROPERTIES LINK_FLAGS "-fsanitize=fuzzer")
	set_target_properties(library_fuzzer PROPERTIES COMPILE_FLAGS "-DANTLR4CPP_STATIC")
    
	configure_file(
                ${PROJECT_SOURCE_DIR}/fuzzer.dict
                ${CMAKE_BINARY_DIR}/bin/fuzzer.dict
				COPYONLY)

    if(UNIX)
	    target_link_libraries(library_fuzzer pthread)
    endif()
	target_link_libraries(library_fuzzer ${ANTLR4_RUNTIME})
	if(FILESYSTEM_LINK)
		target_link_libraries(library_fuzzer ${FILESYSTEM_LIBRARY})
    endif()
    
	add_dependencies(library_fuzzer antlr4jar json)
endif()