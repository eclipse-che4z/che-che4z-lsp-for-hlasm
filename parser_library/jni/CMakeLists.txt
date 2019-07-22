cmake_minimum_required (VERSION 3.10)
project(jni_parser_library)
find_package(JNI REQUIRED)
find_package(Java REQUIRED)
SET (CMAKE_CXX_STANDARD 17)

set(GTEST_EXTERNAL_ROOT ${CMAKE_BINARY_DIR}/externals/googletest)


set (JAVA_SRC
	${PROJECT_SOURCE_DIR}/HlasmPlugin-JNI/src/main/java/com/broadcom/HlasmPlugin/Diagnostic.java
	${PROJECT_SOURCE_DIR}/HlasmPlugin-JNI/src/main/java/com/broadcom/HlasmPlugin/HlasmPlugin.java
	)

add_custom_command(
		OUTPUT
			${PROJECT_SOURCE_DIR}/generated/com_broadcom_HlasmPlugin_HlasmPlugin.h

		COMMAND
			${Java_JAVAC_EXECUTABLE} -h ${PROJECT_SOURCE_DIR}/generated -d ${CMAKE_BINARY_DIR}/bin/ ${JAVA_SRC}
		DEPENDS
			${JAVA_SRC}
		WORKING_DIRECTORY
			${PROJECT_SOURCE_DIR}/HlasmPlugin-JNI/src/main/java/
)

add_custom_target(jni_interface_header
		ALL
		DEPENDS
			${PROJECT_SOURCE_DIR}/generated/com_broadcom_HlasmPlugin_HlasmPlugin.h
)




include(../tag_generated_files.cmake)


add_library(jni_parser_library SHARED
    ${LIB_SRC}
    ${GENERATED_SRC}
	${PROJECT_SOURCE_DIR}/src/hlasm_plugin.cpp
)
if(NOT BUILD_SHARED_LIBS)
	set_target_properties(jni_parser_library PROPERTIES COMPILE_FLAGS "-DANTLR4CPP_STATIC")
endif()

target_link_libraries(jni_parser_library ${ANTLR4_RUNTIME})
if(FILESYSTEM_LINK)
	target_link_libraries(jni_parser_library ${FILESYSTEM_LIBRARY})
endif()

target_include_directories(jni_parser_library
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/../include
		${PROJECT_SOURCE_DIR}/../src/generated/
		${PROJECT_SOURCE_DIR}/../src/
		${PROJECT_SOURCE_DIR}/export
		${PROJECT_SOURCE_DIR}/generated
		${JNI_INCLUDE_DIRS}
) 

add_dependencies(jni_parser_library antlr4jar GenerateParser)
add_dependencies(jni_parser_library json)
add_dependencies(jni_parser_library jni_interface_header)


if(BUILD_TESTING)
	add_custom_target(jni_interface_test
		ALL
		DEPENDS
			${CMAKE_BINARY_DIR}/bin/JNITest.class
	)
	
	add_dependencies(jni_interface_test jni_parser_library)

	add_custom_command(
		OUTPUT
			${CMAKE_BINARY_DIR}/bin/JNITest.class
		COMMAND
			${Java_JAVAC_EXECUTABLE} -d ${CMAKE_BINARY_DIR}/bin/ ${PROJECT_SOURCE_DIR}/test/JNITest.java
		DEPENDS
			${PROJECT_SOURCE_DIR}/test/JNITest.java
		WORKING_DIRECTORY
			${PROJECT_SOURCE_DIR}/HlasmPlugin-JNI/src/main/java/
	)
endif()


