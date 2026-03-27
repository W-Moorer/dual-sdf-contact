#pragma once

#include <string>

#include "baseline/solver/contact_problem.h"

namespace baseline {

class ContactSolver {
 public:
  virtual ~ContactSolver() = default;
  virtual std::string name() const = 0;
  virtual bool available() const { return true; }
  virtual SolverResult solve(const ContactProblem& problem) const = 0;
};

}  // namespace baseline
