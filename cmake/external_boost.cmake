cmake_minimum_required (VERSION 3.10)

project(boost-asio)

include(ExternalProject)

FIND_PACKAGE(Git REQUIRED)
ExternalProject_Add(boost_ext
  PREFIX            ${CMAKE_BINARY_DIR}/externals/boost
  GIT_REPOSITORY    https://github.com/chriskohlhoff/asio.git
  GIT_TAG           eed287d46c14310f0daf4ff19b02979171437232
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
  TEST_COMMAND      ""
)


ExternalProject_Get_Property(boost_ext INSTALL_DIR)
set(BOOST_INCLUDE_DIRS ${INSTALL_DIR}/src/boost_ext/asio/include/)