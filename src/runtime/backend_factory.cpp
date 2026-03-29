#include "baseline/runtime/backend_factory.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include "baseline/core/build_config.h"
#include "baseline/sdf/nanovdb_sdf_provider.h"
#include "baseline/sdf/openvdb_sdf_provider.h"
#include "baseline/solver/simple_ccp_solver.h"
#include "baseline/solver/siconos_solver.h"

namespace baseline {

namespace {

std::string normalizeToken(std::string_view token) {
  std::string normalized(token);
  std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char value) {
    return static_cast<char>(std::tolower(value));
  });
  return normalized;
}

BackendSelectionError unavailableBackendError(std::string_view backend_type,
                                              std::string_view requested_name,
                                              std::string_view reason) {
  std::ostringstream stream;
  stream << "Requested " << backend_type << " backend '" << requested_name
         << "' is not available in this build. " << reason;
  return BackendSelectionError(stream.str());
}

}  // namespace

BackendAvailabilitySummary queryBackendAvailability() {
  BackendAvailabilitySummary summary;
  summary.platform_track = BASELINE_PLATFORM_TRACK;
  summary.default_sdf_backend = BASELINE_DEFAULT_SDF_BACKEND;
  summary.default_reference_backend = BASELINE_DEFAULT_REFERENCE_BACKEND;
  summary.default_solver_backend = BASELINE_DEFAULT_SOLVER_BACKEND;
  summary.force_fallback_sdf = BASELINE_FORCE_FALLBACK_SDF != 0;
  summary.force_fallback_reference = BASELINE_FORCE_FALLBACK_REFERENCE != 0;
  summary.force_simple_solver = BASELINE_FORCE_SIMPLE_SOLVER != 0;
  summary.openvdb_available = BASELINE_REAL_OPENVDB_AVAILABLE != 0;
  summary.nanovdb_available = BASELINE_REAL_NANOVDB_AVAILABLE != 0;
  summary.hppfcl_available = BASELINE_REAL_HPP_FCL_AVAILABLE != 0;
  summary.fcl_available = BASELINE_REAL_FCL_AVAILABLE != 0;
  summary.siconos_available = BASELINE_REAL_SICONOS_AVAILABLE != 0;
  summary.sdf_backends_available.push_back("analytic");
  summary.reference_backends_available.push_back("analytic");
  summary.solver_backends_available.push_back("simple");
  if (summary.openvdb_available) {
    summary.sdf_backends_available.push_back("openvdb");
    summary.real_backends.push_back("openvdb");
  }
  if (summary.nanovdb_available) {
    summary.sdf_backends_available.push_back("nanovdb");
    summary.real_backends.push_back("nanovdb");
  }
  if (summary.hppfcl_available) {
    summary.reference_backends_available.push_back("hppfcl");
    summary.real_backends.push_back("hppfcl");
  }
  if (summary.fcl_available) {
    summary.reference_backends_available.push_back("fcl");
    summary.real_backends.push_back("fcl");
  }
  if (summary.siconos_available) {
    summary.solver_backends_available.push_back("siconos");
    summary.real_backends.push_back("siconos");
  }
  return summary;
}

ResolvedSdfBackend resolveSdfBackend(std::string_view requested_name) {
  const BackendAvailabilitySummary summary = queryBackendAvailability();
  const std::string token = requested_name.empty() ? normalizeToken(summary.default_sdf_backend)
                                                   : normalizeToken(requested_name);

  ResolvedSdfBackend resolved;
  resolved.explicit_request = !requested_name.empty();
  resolved.requested_name = resolved.explicit_request ? std::string(requested_name) : "default";

  if (token == "analytic") {
    resolved.kind = SdfBackendKind::Analytic;
    resolved.selected_name = "analytic";
    resolved.uses_fallback_execution = true;
    resolved.note =
        "Running fallback SDF provider. This is the stable Windows-first baseline path and the default when no real SDF backend is selected.";
    return resolved;
  }
  if (token == "openvdb") {
    if (!summary.openvdb_available) {
      throw unavailableBackendError(
          "SDF", token, "OpenVDB was not detected or BASELINE_WITH_OPENVDB=OFF at configure time.");
    }
    if (summary.force_fallback_sdf) {
      throw unavailableBackendError(
          "SDF", token, "BASELINE_FORCE_FALLBACK_SDF=ON currently pins the build to the analytic SDF path.");
    }
    resolved.kind = SdfBackendKind::OpenVdb;
    resolved.selected_name = "openvdb";
    resolved.uses_fallback_execution = false;
    resolved.note =
        "OpenVDB dependency was detected. Real grid generation and world-space sampling are enabled for sphere/box level sets, with analytic extension outside the trusted narrow band for current primitive-backed examples.";
    return resolved;
  }
  if (token == "nanovdb") {
    if (!summary.nanovdb_available) {
      throw unavailableBackendError(
          "SDF", token, "NanoVDB headers were not detected or BASELINE_WITH_NANOVDB=OFF at configure time.");
    }
    if (summary.force_fallback_sdf) {
      throw unavailableBackendError(
          "SDF", token, "BASELINE_FORCE_FALLBACK_SDF=ON currently pins the build to the analytic SDF path.");
    }
    resolved.kind = SdfBackendKind::NanoVdb;
    resolved.selected_name = "nanovdb";
    resolved.uses_fallback_execution = true;
    resolved.note =
        "NanoVDB remains optional in this stage. The current implementation keeps it in read-only skeleton status and falls back to the analytic narrow-band path.";
    return resolved;
  }

  throw unavailableBackendError("SDF",
                                token,
                                "Supported values are analytic, openvdb, nanovdb.");
}

ResolvedReferenceBackend resolveReferenceBackend(std::string_view requested_name) {
  const BackendAvailabilitySummary summary = queryBackendAvailability();
  const std::string token = requested_name.empty() ? normalizeToken(summary.default_reference_backend)
                                                   : normalizeToken(requested_name);

  ResolvedReferenceBackend resolved;
  resolved.explicit_request = !requested_name.empty();
  resolved.requested_name = resolved.explicit_request ? std::string(requested_name) : "default";

  if (token == "analytic") {
    resolved.kind = ReferenceBackendKind::Analytic;
    resolved.selected_name = "analytic";
    resolved.uses_fallback_execution = true;
    resolved.note =
        "Running fallback reference geometry backend. This is the stable Windows-first path for distance and collision baselines.";
    return resolved;
  }
  if (token == "hppfcl") {
    if (!summary.hppfcl_available) {
      throw unavailableBackendError(
          "reference", token, "hpp-fcl was not detected or BASELINE_WITH_HPP_FCL=OFF at configure time.");
    }
    if (summary.force_fallback_reference) {
      throw unavailableBackendError(
          "reference", token, "BASELINE_FORCE_FALLBACK_REFERENCE=ON currently pins the build to the analytic reference backend.");
    }
    resolved.kind = ReferenceBackendKind::HppFcl;
    resolved.selected_name = "hppfcl";
    resolved.uses_fallback_execution = true;
    resolved.note =
        "hpp-fcl remains a preferred future target, but this stage keeps it in skeleton status and falls back to analytic reference geometry.";
    return resolved;
  }
  if (token == "fcl") {
    if (!summary.fcl_available) {
      throw unavailableBackendError(
          "reference", token, "FCL was not detected or BASELINE_WITH_FCL=OFF at configure time.");
    }
    if (summary.force_fallback_reference) {
      throw unavailableBackendError(
          "reference", token, "BASELINE_FORCE_FALLBACK_REFERENCE=ON currently pins the build to the analytic reference backend.");
    }
    resolved.kind = ReferenceBackendKind::Fcl;
    resolved.selected_name = "fcl";
    resolved.uses_fallback_execution = false;
    resolved.note =
        "FCL dependency was detected. Real distance/collision queries are enabled for sphere/box primitives.";
    return resolved;
  }

  throw unavailableBackendError("reference",
                                token,
                                "Supported values are analytic, hppfcl, fcl.");
}

ResolvedSolverBackend resolveSolverBackend(std::string_view requested_name) {
  const BackendAvailabilitySummary summary = queryBackendAvailability();
  const std::string token = requested_name.empty() ? normalizeToken(summary.default_solver_backend)
                                                   : normalizeToken(requested_name);

  ResolvedSolverBackend resolved;
  resolved.explicit_request = !requested_name.empty();
  resolved.requested_name = resolved.explicit_request ? std::string(requested_name) : "default";

  if (token == "simple") {
    resolved.kind = SolverBackendKind::Simple;
    resolved.selected_name = "simple";
    resolved.uses_fallback_execution = true;
    resolved.note =
        "Running SimpleCcpSolver. This is the default baseline solver and the recommended path on the current Windows-only host.";
    return resolved;
  }
  if (token == "siconos") {
    if (!summary.siconos_available) {
      throw unavailableBackendError(
          "solver", token, "Siconos was not detected or BASELINE_WITH_SICONOS=OFF at configure time.");
    }
    if (summary.force_simple_solver) {
      throw unavailableBackendError(
          "solver", token, "BASELINE_FORCE_SIMPLE_SOLVER=ON currently pins the build to the local SimpleCcpSolver.");
    }
    resolved.kind = SolverBackendKind::Siconos;
    resolved.selected_name = "siconos";
    resolved.uses_fallback_execution = true;
    resolved.note =
        "Siconos dependency was detected. The adapter skeleton is selected, but the current baseline still delegates the solve step to SimpleCcpSolver until fc3d integration is wired in.";
    return resolved;
  }

  throw unavailableBackendError("solver",
                                token,
                                "Supported values are simple, siconos.");
}

std::unique_ptr<SdfProvider> makeSdfProvider(const ResolvedSdfBackend& backend, const SdfBuildInput& input) {
  switch (backend.kind) {
    case SdfBackendKind::Analytic:
      return std::make_unique<AnalyticNarrowBandSdfProvider>(
          input.provider_name, "analytic-fallback", input.geometry, input.voxel_size, input.narrow_band);
    case SdfBackendKind::OpenVdb:
      return std::make_unique<OpenVdbSdfProvider>(
          input.provider_name, input.geometry, input.voxel_size, input.narrow_band);
    case SdfBackendKind::NanoVdb:
      return std::make_unique<NanoVdbSdfProvider>(
          input.provider_name, input.geometry, input.voxel_size, input.narrow_band);
  }
  throw BackendSelectionError("Unsupported SDF backend kind.");
}

std::unique_ptr<ReferenceGeometryQueryEngine> makeReferenceBackend(const ResolvedReferenceBackend& backend) {
  switch (backend.kind) {
    case ReferenceBackendKind::Analytic:
      return std::make_unique<AnalyticReferenceBackend>();
    case ReferenceBackendKind::HppFcl:
      return std::make_unique<HppFclReferenceBackend>();
    case ReferenceBackendKind::Fcl:
      return std::make_unique<FclReferenceBackend>();
  }
  throw BackendSelectionError("Unsupported reference backend kind.");
}

std::unique_ptr<ContactSolver> makeSolverBackend(const ResolvedSolverBackend& backend) {
  switch (backend.kind) {
    case SolverBackendKind::Simple:
      return std::make_unique<SimpleCcpSolver>();
    case SolverBackendKind::Siconos:
      return std::make_unique<OptionalSiconosSolver>();
  }
  throw BackendSelectionError("Unsupported solver backend kind.");
}

std::string joinStrings(const std::vector<std::string>& values, std::string_view delimiter) {
  std::ostringstream stream;
  for (std::size_t index = 0; index < values.size(); ++index) {
    if (index > 0) {
      stream << delimiter;
    }
    stream << values[index];
  }
  return stream.str();
}

std::string buildConfigurationSummary() {
  const BackendAvailabilitySummary summary = queryBackendAvailability();
  std::ostringstream stream;
  stream << "platform track: " << summary.platform_track << "\n"
         << "SDF backend available: " << joinStrings(summary.sdf_backends_available, ", ") << "\n"
         << "reference backend available: " << joinStrings(summary.reference_backends_available, ", ") << "\n"
         << "solver backend available: " << joinStrings(summary.solver_backends_available, ", ") << "\n"
         << "active default backend: sdf=" << summary.default_sdf_backend
         << ", reference=" << summary.default_reference_backend
         << ", solver=" << summary.default_solver_backend << "\n"
         << "fallback enabled: sdf=" << (summary.force_fallback_sdf ? "true" : "false")
         << ", reference=" << (summary.force_fallback_reference ? "true" : "false")
         << ", solver=" << (summary.force_simple_solver ? "true" : "false") << "\n"
         << "real backends available: "
         << (summary.real_backends.empty() ? "none" : joinStrings(summary.real_backends, ", "));
  return stream.str();
}

}  // namespace baseline
