CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(JSONRPCPP_fetcher CXX)
INCLUDE(ExternalProject)
FIND_PACKAGE(Git REQUIRED)

# only JRE required
#FIND_PACKAGE(Java COMPONENTS Runtime REQUIRED)

############ Download and Generate runtime #################
set(JSONRPCPP_EXTERNAL_ROOT ${CMAKE_BINARY_DIR}/externals/jsonrpcppext)

# external repository
set(JSONRPCPP_EXTERNAL_REPO "https://github.com/badaix/jsonrpcpp.git")
set(JSONRPCPP_EXTERNAL_TAG  "v1.1.1")


# download runtime environment
ExternalProject_ADD(
  jsonrpcppext
  PREFIX             ${JSONRPCPP_EXTERNAL_ROOT}
  GIT_REPOSITORY     ${JSONRPCPP_EXTERNAL_REPO}
  GIT_TAG            ${JSONRPCPP_EXTERNAL_TAG}
  TIMEOUT            10
  LOG_DOWNLOAD       ON
  GIT_PROGRESS       1
  CMAKE_ARGS -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
)

ExternalProject_Get_Property(jsonrpcppext INSTALL_DIR)



list(APPEND JSONRPCPP_INCLUDE_DIRS ${INSTALL_DIR}/include/jsonrpcpp)

set(JSONRPCPP_LIBS "${INSTALL_DIR}/lib")


