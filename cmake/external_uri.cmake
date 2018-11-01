CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(uri_fetcher CXX)
INCLUDE(ExternalProject)
FIND_PACKAGE(Git REQUIRED)

# download runtime environment
ExternalProject_ADD(
  uri_ext
  PREFIX             ${CMAKE_BINARY_DIR}/externals/uri
  GIT_REPOSITORY     "https://github.com/cpp-netlib/uri.git"
  GIT_TAG            1.0.1
  TIMEOUT            10
  LOG_DOWNLOAD       ON
  GIT_PROGRESS       1
  CMAKE_ARGS  -DUri_BUILD_DOCS=OFF -DUri_BUILD_TESTS=OFF -DUri_USE_STATIC_CRT=OFF -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DUri_WARNINGS_AS_ERRORS=OFF
)

ExternalProject_Get_Property(uri_ext INSTALL_DIR)



set(URI_INCLUDE_DIRS ${INSTALL_DIR}/include/)

set(URI_LIBS "${INSTALL_DIR}/lib")


