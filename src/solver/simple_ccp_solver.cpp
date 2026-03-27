#include "baseline/solver/simple_ccp_solver.h"

#include <cmath>

namespace baseline {

namespace {

double l2Distance(const Vector3& a, const Vector3& b) {
  const double dx = a[0] - b[0];
  const double dy = a[1] - b[1];
  const double dz = a[2] - b[2];
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

Vector3 matrixVectorProduct(const Matrix3& matrix, const Vector3& vector) {
  return {
      matrix[0][0] * vector[0] + matrix[0][1] * vector[1] + matrix[0][2] * vector[2],
      matrix[1][0] * vector[0] + matrix[1][1] * vector[1] + matrix[1][2] * vector[2],
      matrix[2][0] * vector[0] + matrix[2][1] * vector[1] + matrix[2][2] * vector[2],
  };
}

Vector3 add(const Vector3& a, const Vector3& b) { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }

}  // namespace

SimpleCcpSolver::SimpleCcpSolver(SimpleCcpSolverOptions options) : options_(options) {}

std::string SimpleCcpSolver::name() const { return "simple-pgs-ccp"; }

SolverResult SimpleCcpSolver::solve(const ContactProblem& problem) const {
  Vector3 lambda{0.0, 0.0, 0.0};
  double residual = 0.0;
  int iterations = 0;

  for (int iteration = 0; iteration < options_.max_iterations; ++iteration) {
    ++iterations;
    const Vector3 previous = lambda;

    for (int row = 0; row < 3; ++row) {
      double local_residual = problem.free_velocity[row];
      for (int column = 0; column < 3; ++column) {
        local_residual += problem.delassus[row][column] * lambda[column];
      }
      const double diagonal = (problem.delassus[row][row] > 1e-12) ? problem.delassus[row][row] : 1e-12;
      lambda[row] -= options_.relaxation * local_residual / diagonal;
      if (row == 0) {
        lambda[0] = std::max(0.0, lambda[0]);
      }
    }

    const double friction_box_bound = problem.friction_coefficient * lambda[0] / std::sqrt(2.0);
    lambda[1] = clamp(lambda[1], -friction_box_bound, friction_box_bound);
    lambda[2] = clamp(lambda[2], -friction_box_bound, friction_box_bound);

    residual = l2Distance(lambda, previous);
    if (residual < options_.tolerance) {
      break;
    }
  }

  SolverResult result;
  result.converged = residual < options_.tolerance;
  result.impulse = lambda;
  result.post_velocity = add(problem.free_velocity, matrixVectorProduct(problem.delassus, lambda));
  result.iterations = iterations;
  result.residual = residual;
  result.solver_name = name();
  result.note = "Projected Gauss-Seidel on a 3x3 single-contact problem with a box-linearized friction cone.";
  return result;
}

}  // namespace baseline
