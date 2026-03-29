#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "baseline/contact/reference_geometry.h"
#include "baseline/sdf/sdf_provider.h"
#include "baseline/solver/contact_solver.h"

namespace baseline {

enum class SdfBackendKind { Analytic, OpenVdb, NanoVdb };
enum class ReferenceBackendKind { Analytic, HppFcl, Fcl };
enum class SolverBackendKind { Simple, Siconos };

struct BackendAvailabilitySummary {
  std::string platform_track;
  std::string default_sdf_backend;
  std::string default_reference_backend;
  std::string default_solver_backend;
  bool force_fallback_sdf{false};
  bool force_fallback_reference{false};
  bool force_simple_solver{false};
  bool openvdb_available{false};
  bool nanovdb_available{false};
  bool hppfcl_available{false};
  bool fcl_available{false};
  bool siconos_available{false};
  std::vector<std::string> sdf_backends_available;
  std::vector<std::string> reference_backends_available;
  std::vector<std::string> solver_backends_available;
  std::vector<std::string> real_backends;
};

struct ResolvedSdfBackend {
  SdfBackendKind kind{SdfBackendKind::Analytic};
  std::string requested_name;
  std::string selected_name;
  bool explicit_request{false};
  bool uses_fallback_execution{true};
  std::string note;
};

struct ResolvedReferenceBackend {
  ReferenceBackendKind kind{ReferenceBackendKind::Analytic};
  std::string requested_name;
  std::string selected_name;
  bool explicit_request{false};
  bool uses_fallback_execution{true};
  std::string note;
};

struct ResolvedSolverBackend {
  SolverBackendKind kind{SolverBackendKind::Simple};
  std::string requested_name;
  std::string selected_name;
  bool explicit_request{false};
  bool uses_fallback_execution{true};
  std::string note;
};

struct SdfBuildInput {
  std::string provider_name;
  ReferenceGeometry geometry;
  double voxel_size{0.1};
  double narrow_band{2.0};
};

class BackendSelectionError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

BackendAvailabilitySummary queryBackendAvailability();
ResolvedSdfBackend resolveSdfBackend(std::string_view requested_name = {});
ResolvedReferenceBackend resolveReferenceBackend(std::string_view requested_name = {});
ResolvedSolverBackend resolveSolverBackend(std::string_view requested_name = {});

std::unique_ptr<SdfProvider> makeSdfProvider(const ResolvedSdfBackend& backend, const SdfBuildInput& input);
std::unique_ptr<ReferenceGeometryQueryEngine> makeReferenceBackend(const ResolvedReferenceBackend& backend);
std::unique_ptr<ContactSolver> makeSolverBackend(const ResolvedSolverBackend& backend);

std::string buildConfigurationSummary();
std::string joinStrings(const std::vector<std::string>& values, std::string_view delimiter);

}  // namespace baseline
