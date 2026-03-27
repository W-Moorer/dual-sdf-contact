#pragma once

#include "baseline/solver/simple_ccp_solver.h"

namespace baseline {

class OptionalSiconosSolver final : public ContactSolver {
 public:
  OptionalSiconosSolver();

  std::string name() const override;
  bool available() const override;
  SolverResult solve(const ContactProblem& problem) const override;

  static bool realBackendAvailable();
  static std::string availabilitySummary();

 private:
  SimpleCcpSolver fallback_;
};

}  // namespace baseline
