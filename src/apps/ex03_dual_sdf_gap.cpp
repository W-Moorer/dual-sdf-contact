#include <iostream>
#include <sstream>

#include "baseline/contact/dual_sdf_contact_calculator.h"
#include "baseline/runtime/backend_factory.h"
#include "example_utils.h"

int main(int argc, char** argv) {
  using namespace baseline;

  try {
    const std::string example_name = "ex03_dual_sdf_gap";
    const auto options = apps::parseBackendOptions(argc, argv);
    if (options.help_requested) {
      apps::printUsage(example_name);
      return 0;
    }

    const auto analytic_backend = resolveSdfBackend("analytic");
    const auto sdf_backend = resolveSdfBackend(options.sdf_backend);
    const auto output_dir = apps::outputDir(example_name);

    const ReferenceGeometry sphere_a_geometry = ReferenceGeometry::makeSphere("sphere_a", {0.0, 0.0, 0.0}, 1.0);
    const ReferenceGeometry sphere_b_geometry = ReferenceGeometry::makeSphere("sphere_b", {1.7, 0.1, 0.0}, 0.8);

    auto analytic_a = makeSdfProvider(analytic_backend, {"sphere_a", sphere_a_geometry, 0.1, 3.0});
    auto analytic_b = makeSdfProvider(analytic_backend, {"sphere_b", sphere_b_geometry, 0.1, 3.0});
    auto selected_a = makeSdfProvider(sdf_backend, {"sphere_a", sphere_a_geometry, 0.1, 3.0});
    auto selected_b = makeSdfProvider(sdf_backend, {"sphere_b", sphere_b_geometry, 0.1, 3.0});

    const DualSdfContactCalculator calculator;
    const ContactKinematicsResult analytic_result = calculator.compute(*analytic_a, *analytic_b);
    const ContactKinematicsResult selected_result = calculator.compute(*selected_a, *selected_b);

    const double normal_angle_error_deg = apps::normalAngleDegrees(analytic_result.normal, selected_result.normal);

    apps::writeCsv(
        output_dir / "contact.csv",
        {"backend", "signed_gap", "normal_x", "normal_y", "normal_z", "contact_x", "contact_y", "contact_z",
         "symmetry_residual", "tangent_orthogonality", "gradient_alignment", "valid_flags"},
        {
            {"analytic",
             formatDouble(analytic_result.signed_gap),
             formatDouble(analytic_result.normal.x),
             formatDouble(analytic_result.normal.y),
             formatDouble(analytic_result.normal.z),
             formatDouble(analytic_result.contact_point.x),
             formatDouble(analytic_result.contact_point.y),
             formatDouble(analytic_result.contact_point.z),
             formatDouble(analytic_result.symmetry_residual),
             formatDouble(analytic_result.tangent_orthogonality),
             formatDouble(analytic_result.gradient_alignment),
             std::to_string(analytic_result.valid_flags)},
            {selected_result.object_a == analytic_result.object_a && sdf_backend.selected_name == "analytic" ? "analytic"
                                                                                                            : sdf_backend.selected_name,
             formatDouble(selected_result.signed_gap),
             formatDouble(selected_result.normal.x),
             formatDouble(selected_result.normal.y),
             formatDouble(selected_result.normal.z),
             formatDouble(selected_result.contact_point.x),
             formatDouble(selected_result.contact_point.y),
             formatDouble(selected_result.contact_point.z),
             formatDouble(selected_result.symmetry_residual),
             formatDouble(selected_result.tangent_orthogonality),
             formatDouble(selected_result.gradient_alignment),
             std::to_string(selected_result.valid_flags)},
        });

    std::ostringstream summary;
    summary << "{\n"
            << "  \"example\": " << quoteJson(example_name) << ",\n"
            << "  \"selected_sdf_backend\": " << quoteJson(sdf_backend.selected_name) << ",\n"
            << "  \"analytic_backend\": " << quoteJson(analytic_a->backendName()) << ",\n"
            << "  \"selected_backend\": " << quoteJson(selected_a->backendName()) << ",\n"
            << "  \"backend_note\": " << quoteJson(sdf_backend.note) << ",\n"
            << "  \"analytic\": {\n"
            << "    \"signed_gap\": " << formatDouble(analytic_result.signed_gap) << ",\n"
            << "    \"normal\": " << apps::vec3Json(analytic_result.normal) << ",\n"
            << "    \"symmetry_residual\": " << formatDouble(analytic_result.symmetry_residual) << ",\n"
            << "    \"tangent_orthogonality\": " << formatDouble(analytic_result.tangent_orthogonality) << ",\n"
            << "    \"point_on_a\": " << apps::vec3Json(analytic_result.point_on_a) << ",\n"
            << "    \"point_on_b\": " << apps::vec3Json(analytic_result.point_on_b) << "\n"
            << "  },\n"
            << "  \"selected\": {\n"
            << "    \"signed_gap\": " << formatDouble(selected_result.signed_gap) << ",\n"
            << "    \"normal\": " << apps::vec3Json(selected_result.normal) << ",\n"
            << "    \"symmetry_residual\": " << formatDouble(selected_result.symmetry_residual) << ",\n"
            << "    \"tangent_orthogonality\": " << formatDouble(selected_result.tangent_orthogonality) << ",\n"
            << "    \"gradient_alignment\": " << formatDouble(selected_result.gradient_alignment) << ",\n"
            << "    \"point_on_a\": " << apps::vec3Json(selected_result.point_on_a) << ",\n"
            << "    \"point_on_b\": " << apps::vec3Json(selected_result.point_on_b) << ",\n"
            << "    \"valid_flags\": " << selected_result.valid_flags << "\n"
            << "  },\n"
            << "  \"gap_error\": " << formatDouble(std::abs(selected_result.signed_gap - analytic_result.signed_gap)) << ",\n"
            << "  \"normal_angle_error_deg\": " << formatDouble(normal_angle_error_deg) << "\n"
            << "}\n";
    writeTextFile(output_dir / "contact.json", summary.str());

    std::cout << example_name << ": sdf_backend=" << sdf_backend.selected_name
              << ", signed_gap=" << selected_result.signed_gap
              << ", symmetry_residual=" << selected_result.symmetry_residual << "\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "ex03_dual_sdf_gap failed: " << error.what() << "\n";
    return 1;
  }
}
