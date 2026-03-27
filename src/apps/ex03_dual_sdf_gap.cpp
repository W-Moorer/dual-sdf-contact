#include <iostream>
#include <sstream>

#include "baseline/contact/dual_sdf_contact_calculator.h"
#include "baseline/sdf/nanovdb_sdf_provider.h"
#include "baseline/sdf/openvdb_sdf_provider.h"
#include "example_utils.h"

int main() {
  using namespace baseline;

  const std::string example_name = "ex03_dual_sdf_gap";
  const auto output_dir = apps::outputDir(example_name);

  const NanoVdbSdfProvider sphere_a =
      NanoVdbSdfProvider::makeSphere("sphere_a", {0.0, 0.0, 0.0}, 1.0, 0.1, 3.0);
  const OpenVdbSdfProvider sphere_b =
      OpenVdbSdfProvider::makeSphere("sphere_b", {1.7, 0.1, 0.0}, 0.8, 0.1, 3.0);

  const DualSdfContactCalculator calculator;
  const ContactGeometry contact = calculator.compute(sphere_a, sphere_b);

  apps::writeCsv(
      output_dir / "contact.csv",
      {"signed_gap", "normal_x", "normal_y", "normal_z", "contact_x", "contact_y", "contact_z"},
      {{
          formatDouble(contact.signed_gap),
          formatDouble(contact.frame.normal.x),
          formatDouble(contact.frame.normal.y),
          formatDouble(contact.frame.normal.z),
          formatDouble(contact.contact_point.x),
          formatDouble(contact.contact_point.y),
          formatDouble(contact.contact_point.z),
      }});

  std::ostringstream summary;
  summary << "{\n"
          << "  \"example\": " << quoteJson(example_name) << ",\n"
          << "  \"object_a_backend\": " << quoteJson(sphere_a.backendName()) << ",\n"
          << "  \"object_b_backend\": " << quoteJson(sphere_b.backendName()) << ",\n"
          << "  \"signed_gap\": " << formatDouble(contact.signed_gap) << ",\n"
          << "  \"normal\": " << apps::vec3Json(contact.frame.normal) << ",\n"
          << "  \"tangent_u\": " << apps::vec3Json(contact.frame.tangent_u) << ",\n"
          << "  \"tangent_v\": " << apps::vec3Json(contact.frame.tangent_v) << ",\n"
          << "  \"support_point_a\": " << apps::vec3Json(contact.support_point_a) << ",\n"
          << "  \"support_point_b\": " << apps::vec3Json(contact.support_point_b) << "\n"
          << "}\n";
  writeTextFile(output_dir / "contact.json", summary.str());

  std::cout << example_name << ": signed_gap=" << contact.signed_gap
            << ", normal=" << formatVec3(contact.frame.normal) << "\n";
  return 0;
}
