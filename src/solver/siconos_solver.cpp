#include "baseline/solver/siconos_solver.h"

namespace baseline {

OptionalSiconosSolver::OptionalSiconosSolver() = default;

std::string OptionalSiconosSolver::name() const { return "optional-siconos"; }

bool OptionalSiconosSolver::available() const { return BASELINE_HAS_SICONOS; }

SolverResult OptionalSiconosSolver::solve(const ContactProblem& problem) const {
  SolverResult result = fallback_.solve(problem);
  result.solver_name = name();
  if (available()) {
    result.note =
        "Siconos package was detected, but this baseline still routes through the local SimpleCcpSolver adapter hook.";
  } else {
    result.note = "Siconos was not detected at configure time; returned the local SimpleCcpSolver baseline instead.";
  }
  return result;
}

}  // namespace baseline
