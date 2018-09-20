cmake_minimum_required (VERSION 3.10)


project(googletest-download NONE)

set(GTEST_EXTERNAL_ROOT ${CMAKE_BINARY_DIR}/externals/googletest)

include(ExternalProject)
ExternalProject_Add(googletest
  GIT_REPOSITORY    https://github.com/google/googletest.git
  GIT_TAG           880896c6f4814f4c7798355a652dc6167be2b75f
  SOURCE_DIR        ${CMAKE_BINARY_DIR}/googletest-src
  BINARY_DIR        ${CMAKE_BINARY_DIR}/googletest-build
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
  TEST_COMMAND      ""
)
