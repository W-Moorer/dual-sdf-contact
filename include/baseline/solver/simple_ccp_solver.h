#pragma once

#include "baseline/solver/contact_solver.h"

namespace baseline {

struct SimpleCcpSolverOptions {
  int max_iterations{100};
  double tolerance{1e-8};
  double relaxation{1.0};
};

class SimpleCcpSolver final : public ContactSolver {
 public:
  explicit SimpleCcpSolver(SimpleCcpSolverOptions options = {});

  std::string name() const override;
  SolverResult solve(const ContactProblem& problem) const override;

 private:
  SimpleCcpSolverOptions options_;
};

}  // namespace baseline
