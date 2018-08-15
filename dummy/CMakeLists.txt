cmake_minimum_required (VERSION 3.10)

PROJECT(dummy)

# compile sources to the executable
add_executable(dummy 
	${PROJECT_SOURCE_DIR}/src/main.cpp
	)

# link executable with libraries
target_link_libraries(dummy  antlr4-runtime)
target_link_libraries(dummy  parser_library)
