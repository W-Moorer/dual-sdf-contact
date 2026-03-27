#include <cmath>
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

    const auto sdf_backend = resolveSdfBackend(options.sdf_backend);
    const auto output_dir = apps::outputDir(example_name);

    auto sphere_a = makeSdfProvider(
        sdf_backend,
        {"sphere_a", ReferenceGeometry::makeSphere("sphere_a", {0.0, 0.0, 0.0}, 1.0), 0.1, 3.0});
    auto sphere_b = makeSdfProvider(
        sdf_backend,
        {"sphere_b", ReferenceGeometry::makeSphere("sphere_b", {1.7, 0.1, 0.0}, 0.8), 0.1, 3.0});

    const DualSdfContactCalculator calculator;
    const ContactGeometry contact_ab = calculator.compute(*sphere_a, *sphere_b);
    const ContactGeometry contact_ba = calculator.compute(*sphere_b, *sphere_a);
    const double symmetry_residual =
        std::abs(contact_ab.signed_gap - contact_ba.signed_gap) +
        norm(contact_ab.contact_point - contact_ba.contact_point) +
        std::abs(dot(contact_ab.frame.normal, contact_ba.frame.normal) + 1.0);

    apps::writeCsv(
        output_dir / "contact.csv",
        {"signed_gap", "normal_x", "normal_y", "normal_z", "contact_x", "contact_y", "contact_z",
         "symmetry_residual"},
        {{
            formatDouble(contact_ab.signed_gap),
            formatDouble(contact_ab.frame.normal.x),
            formatDouble(contact_ab.frame.normal.y),
            formatDouble(contact_ab.frame.normal.z),
            formatDouble(contact_ab.contact_point.x),
            formatDouble(contact_ab.contact_point.y),
            formatDouble(contact_ab.contact_point.z),
            formatDouble(symmetry_residual),
        }});

    std::ostringstream summary;
    summary << "{\n"
            << "  \"example\": " << quoteJson(example_name) << ",\n"
            << "  \"selected_sdf_backend\": " << quoteJson(sdf_backend.selected_name) << ",\n"
            << "  \"object_a_backend\": " << quoteJson(sphere_a->backendName()) << ",\n"
            << "  \"object_b_backend\": " << quoteJson(sphere_b->backendName()) << ",\n"
            << "  \"backend_note\": " << quoteJson(sdf_backend.note) << ",\n"
            << "  \"signed_gap\": " << formatDouble(contact_ab.signed_gap) << ",\n"
            << "  \"symmetry_residual\": " << formatDouble(symmetry_residual) << ",\n"
            << "  \"normal\": " << apps::vec3Json(contact_ab.frame.normal) << ",\n"
            << "  \"tangent_u\": " << apps::vec3Json(contact_ab.frame.tangent_u) << ",\n"
            << "  \"tangent_v\": " << apps::vec3Json(contact_ab.frame.tangent_v) << ",\n"
            << "  \"support_point_a\": " << apps::vec3Json(contact_ab.support_point_a) << ",\n"
            << "  \"support_point_b\": " << apps::vec3Json(contact_ab.support_point_b) << "\n"
            << "}\n";
    writeTextFile(output_dir / "contact.json", summary.str());

    std::cout << example_name << ": sdf_backend=" << sdf_backend.selected_name
              << ", signed_gap=" << contact_ab.signed_gap
              << ", symmetry_residual=" << symmetry_residual << "\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "ex03_dual_sdf_gap failed: " << error.what() << "\n";
    return 1;
  }
}
