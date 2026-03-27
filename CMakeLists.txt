cmake_minimum_required(VERSION 3.22)

project(
  dual_sdf_contact
  VERSION 0.1.0
  DESCRIPTION "Minimal baseline platform for dual-SDF contact dynamics experiments."
  LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(BaselineOptions)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/generated/baseline/core")

configure_file(
  "${PROJECT_SOURCE_DIR}/include/baseline/core/build_config.h.in"
  "${PROJECT_BINARY_DIR}/generated/baseline/core/build_config.h"
  @ONLY)

add_subdirectory(src)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
