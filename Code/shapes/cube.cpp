//
// Created by alex on 29/10/2025.
//

#include "cube.h"
#include <limits>
#include <cmath>
#include <algorithm>
#include "../Image.h"

// Helper function to update min/max bounds based on a point
static void updateBounds(const Vector3& p, Vector3& min_p, Vector3& max_p) {
    min_p.x = std::min(min_p.x, p.x);
    min_p.y = std::min(min_p.y, p.y);
    min_p.z = std::min(min_p.z, p.z);
    max_p.x = std::max(max_p.x, p.x);
    max_p.y = std::max(max_p.y, p.y);
    max_p.z = std::max(max_p.z, p.z);
}

bool Cube::getBoundingBox(AABB& output_box) const {
    double infinity = std::numeric_limits<double>::infinity();
    Vector3 min_p(infinity, infinity, infinity);
    Vector3 max_p(-infinity, -infinity, -infinity);

    // 1. Calculate the box for the cube at its initial position (t=0)
    // We transform the 8 corners of the canonical unit cube (-1 to 1)
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            for (int k = 0; k < 2; ++k) {
                Vector3 corner(
                    (i == 0) ? -1.0 : 1.0,
                    (j == 0) ? -1.0 : 1.0,
                    (k == 0) ? -1.0 : 1.0
                );
                Vector3 transformed_corner = m_transform * corner;
                updateBounds(transformed_corner, min_p, max_p);
            }
        }
    }

    AABB box_t0(min_p, max_p);

    // 2. Account for Motion Blur (Velocity)
    Vector3 min_p_t1 = min_p + m_velocity;
    Vector3 max_p_t1 = max_p + m_velocity;
    AABB box_t1(min_p_t1, max_p_t1);

    output_box = AABB::combine(box_t0, box_t1);

    return true;
}

bool Cube::intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const {

    Vector3 inverseRay_origin = m_inverse_transform * ray.origin;
    Vector3 inverseRay_direction = m_inverse_transform.transformDirection(ray.direction);

    double x0 = inverseRay_origin.x;
    double dx = inverseRay_direction.x;
    double y0 = inverseRay_origin.y;
    double dy = inverseRay_direction.y;
    double z0 = inverseRay_origin.z;
    double dz = inverseRay_direction.z;

    bool hit_found = false;
    double closest_t = t_max;
    const double par_epsilon = 1e-6;
    const double bound_epsilon = 1e-6;

    auto check_face = [&](double t_val, const Vector3& local_normal, int axis_u, int axis_v) {

        if (t_val > t_min && t_val < closest_t) {
            Vector3 p = inverseRay_origin + (t_val * inverseRay_direction);

            double u = (axis_u == 0) ? p.x : (axis_u == 1) ? p.y : p.z;
            double v = (axis_v == 0) ? p.x : (axis_v == 1) ? p.y : p.z;

            // Check if within the unit square [-1, 1] with a small epsilon buffer
            if (u > -(1.0 + bound_epsilon) && u < (1.0 + bound_epsilon) &&
                v > -(1.0 + bound_epsilon) && v < (1.0 + bound_epsilon)) {
                hit_found = true;
                closest_t = t_val;

                rec.t = t_val;
                rec.mat = m_material;

                rec.point = m_transform * p;

                Vector3 world_normal = m_inverse_transpose.transformDirection(local_normal);
                world_normal = world_normal.normalize();
                rec.set_face_normal(ray, world_normal);

                rec.u = (u + 1.0) * 0.5;
                rec.v = (v + 1.0) * 0.5;
            }

        }
    };

    // Check X-planes (if not parallel)
    if (std::abs(dx) > par_epsilon) {
        double tx0 = (-1 - x0) / dx; // Left face (-X)
        double tx1 = (1 - x0) / dx;  // Right face (+X)
        // Axis indices: 1=y, 2=z
        check_face(tx0, Vector3(-1, 0, 0), 1, 2);
        check_face(tx1, Vector3(1, 0, 0),  1, 2);
    }

    // Check Y-planes
    if (std::abs(dy) > par_epsilon) {
        double ty0 = (-1 - y0) / dy; // Bottom face (-Y)
        double ty1 = (1 - y0) / dy;  // Top face (+Y)
        // Axis indices: 0=x, 2=z
        check_face(ty0, Vector3(0, -1, 0), 0, 2);
        check_face(ty1, Vector3(0, 1, 0),  0, 2);
    }

    // Check Z-planes
    if (std::abs(dz) > par_epsilon) {
        double tz0 = (-1 - z0) / dz; // Back face (-Z)
        double tz1 = (1 - z0) / dz;  // Front face (+Z)
        // Axis indices: 0=x, 1=y
        check_face(tz0, Vector3(0, 0, -1), 0, 1);
        check_face(tz1, Vector3(0, 0, 1),  0, 1);
    }

    return hit_found;
}