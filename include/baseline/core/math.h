#pragma once

#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

namespace baseline {

struct Vec3 {
  double x{0.0};
  double y{0.0};
  double z{0.0};
};

struct Aabb3 {
  Vec3 lower{0.0, 0.0, 0.0};
  Vec3 upper{0.0, 0.0, 0.0};
  bool valid{false};

  bool contains(const Vec3& point, double tolerance = 0.0) const {
    if (!valid) {
      return false;
    }
    return point.x >= lower.x - tolerance && point.x <= upper.x + tolerance &&
           point.y >= lower.y - tolerance && point.y <= upper.y + tolerance &&
           point.z >= lower.z - tolerance && point.z <= upper.z + tolerance;
  }

  Vec3 center() const {
    return {
        0.5 * (lower.x + upper.x),
        0.5 * (lower.y + upper.y),
        0.5 * (lower.z + upper.z),
    };
  }

  Vec3 extent() const {
    return {
        upper.x - lower.x,
        upper.y - lower.y,
        upper.z - lower.z,
    };
  }
};

inline Vec3 operator+(const Vec3& a, const Vec3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(const Vec3& a, const Vec3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator-(const Vec3& v) { return {-v.x, -v.y, -v.z}; }
inline Vec3 operator*(const Vec3& v, double s) { return {v.x * s, v.y * s, v.z * s}; }
inline Vec3 operator*(double s, const Vec3& v) { return v * s; }
inline Vec3 operator/(const Vec3& v, double s) { return {v.x / s, v.y / s, v.z / s}; }
inline Vec3& operator+=(Vec3& a, const Vec3& b) {
  a.x += b.x;
  a.y += b.y;
  a.z += b.z;
  return a;
}

inline double dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

inline Vec3 cross(const Vec3& a, const Vec3& b) {
  return {
      a.y * b.z - a.z * b.y,
      a.z * b.x - a.x * b.z,
      a.x * b.y - a.y * b.x,
  };
}

inline double squaredNorm(const Vec3& v) { return dot(v, v); }
inline double norm(const Vec3& v) { return std::sqrt(squaredNorm(v)); }

inline Vec3 normalized(const Vec3& v, const Vec3& fallback = {1.0, 0.0, 0.0}) {
  const double n = norm(v);
  if (n <= 1e-12) {
    return fallback;
  }
  return v / n;
}

inline double clamp(double value, double lower, double upper) {
  if (value < lower) {
    return lower;
  }
  if (value > upper) {
    return upper;
  }
  return value;
}

struct ContactFrame {
  Vec3 normal;
  Vec3 tangent_u;
  Vec3 tangent_v;

  bool isOrthonormal(double tolerance = 1e-6) const {
    const double n_n = std::abs(norm(normal) - 1.0);
    const double n_u = std::abs(norm(tangent_u) - 1.0);
    const double n_v = std::abs(norm(tangent_v) - 1.0);
    const double o_nu = std::abs(dot(normal, tangent_u));
    const double o_nv = std::abs(dot(normal, tangent_v));
    const double o_uv = std::abs(dot(tangent_u, tangent_v));
    return n_n < tolerance && n_u < tolerance && n_v < tolerance && o_nu < tolerance &&
           o_nv < tolerance && o_uv < tolerance;
  }
};

inline ContactFrame makeContactFrame(const Vec3& preferred_normal) {
  const Vec3 normal = normalized(preferred_normal, {0.0, 1.0, 0.0});
  const Vec3 helper = (std::abs(normal.z) < 0.9) ? Vec3{0.0, 0.0, 1.0} : Vec3{0.0, 1.0, 0.0};
  const Vec3 tangent_u = normalized(cross(helper, normal), {1.0, 0.0, 0.0});
  const Vec3 tangent_v = normalized(cross(normal, tangent_u), {0.0, 0.0, 1.0});
  return {normal, tangent_u, tangent_v};
}

inline double contactFrameOrthogonalityResidual(const ContactFrame& frame) {
  return std::abs(dot(frame.normal, frame.tangent_u)) + std::abs(dot(frame.normal, frame.tangent_v)) +
         std::abs(dot(frame.tangent_u, frame.tangent_v)) + std::abs(norm(frame.normal) - 1.0) +
         std::abs(norm(frame.tangent_u) - 1.0) + std::abs(norm(frame.tangent_v) - 1.0);
}

inline std::array<double, 3> toLocal(const ContactFrame& frame, const Vec3& world) {
  return {dot(world, frame.normal), dot(world, frame.tangent_u), dot(world, frame.tangent_v)};
}

inline Vec3 fromLocal(const ContactFrame& frame, const std::array<double, 3>& local) {
  return frame.normal * local[0] + frame.tangent_u * local[1] + frame.tangent_v * local[2];
}

inline std::string formatVec3(const Vec3& value, int precision = 6) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(precision) << "[" << value.x << ", " << value.y << ", "
         << value.z << "]";
  return stream.str();
}

}  // namespace baseline
