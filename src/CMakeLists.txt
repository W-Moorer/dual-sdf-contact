set(BASELINE_LIBRARY_SOURCES
  core/io_utils.cpp
  runtime/backend_factory.cpp
  sdf/sdf_provider.cpp
  sdf/openvdb_sdf_provider.cpp
  sdf/nanovdb_sdf_provider.cpp
  contact/reference_geometry.cpp
  contact/candidate_pair_manager.cpp
  contact/dual_sdf_contact_calculator.cpp
  contact/contact_problem_builder.cpp
  solver/simple_ccp_solver.cpp
  solver/siconos_solver.cpp)

add_library(baseline_core STATIC ${BASELINE_LIBRARY_SOURCES})
add_library(baseline::core ALIAS baseline_core)

target_include_directories(
  baseline_core
  PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_BINARY_DIR}/generated")

target_compile_features(baseline_core PUBLIC cxx_std_17)

if(BASELINE_HAS_EIGEN)
  target_link_libraries(baseline_core PUBLIC Eigen3::Eigen)
endif()

if(BASELINE_HAS_OPENVDB AND BASELINE_OPENVDB_TARGET)
  target_link_libraries(baseline_core PUBLIC ${BASELINE_OPENVDB_TARGET})
endif()

if(BASELINE_HAS_FCL AND BASELINE_FCL_TARGET)
  target_link_libraries(baseline_core PUBLIC ${BASELINE_FCL_TARGET})
endif()

if(MSVC)
  target_compile_options(baseline_core PRIVATE /W4 /permissive- /EHsc)
else()
  target_compile_options(baseline_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

function(baseline_set_output_dirs target_name)
  set_target_properties(
    "${target_name}"
    PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin"
      RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/bin/Debug"
      RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}/bin/Release"
      RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PROJECT_BINARY_DIR}/bin/RelWithDebInfo"
      RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${PROJECT_BINARY_DIR}/bin/MinSizeRel")
endfunction()

baseline_set_output_dirs(baseline_core)

function(baseline_add_app target_name)
  add_executable("${target_name}" "apps/${target_name}.cpp")
  target_link_libraries("${target_name}" PRIVATE baseline::core)
  baseline_set_output_dirs("${target_name}")
  set_target_properties("${target_name}" PROPERTIES FOLDER "apps")
endfunction()

baseline_add_app(ex01_nanovdb_hello)
baseline_add_app(ex02_hppfcl_distance)
baseline_add_app(ex03_dual_sdf_gap)
baseline_add_app(ex04_single_step_contact)
baseline_add_app(ex05_compare_backends)
baseline_add_app(ex06_regression_smoke)
