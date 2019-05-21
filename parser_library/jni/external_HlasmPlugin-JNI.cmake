cmake_minimum_required (VERSION 3.10)

include(ExternalProject)
ExternalProject_Add(HlasmPlugin-JNI-download
  GIT_REPOSITORY    https://github.gwd.broadcom.net/mb890989/HlasmPlugin-JNI.git
  #GIT_TAG           880896c6f4814f4c7798355a652dc6167be2b75f
  SOURCE_DIR        ${CMAKE_BINARY_DIR}/externals/HlasmPlugin-JNI
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
  TEST_COMMAND      ""
)