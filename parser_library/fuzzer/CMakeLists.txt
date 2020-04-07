
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