#include "../shapes/cube.h"
#include <cmath>
#include <algorithm> // For std::min and std::max
#include <iostream>
#include <limits>

static void updateBounds(const Vector3& p, Vector3& min_p, Vector3& max_p) {
    min_p.x = std::min(min_p.x, p.x);
    min_p.y = std::min(min_p.y, p.y);
    min_p.z = std::min(min_p.z, p.z);
    max_p.x = std::max(max_p.x, p.x);
    max_p.y = std::max(max_p.y, p.y);
    max_p.z = std::max(max_p.z, p.z);
}

bool Cube::getBoundingBox(AABB& output_box) const {
    // Start with invalid bounds
    double infinity = std::numeric_limits<double>::infinity();
    Vector3 min_p(infinity, infinity, infinity);
    Vector3 max_p(-infinity, -infinity, -infinity);

    // Transform the 8 corners of the unit cube's bounding box (-1 to 1)
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            for (int k = 0; k < 2; ++k) {
                Vector3 corner(
                    (i == 0) ? -1.0 : 1.0,
                    (j == 0) ? -1.0 : 1.0,
                    (k == 0) ? -1.0 : 1.0
                );
                Vector3 transformed_corner = m_transform * corner; // Transform point
                updateBounds(transformed_corner, min_p, max_p);
            }
        }
    }

    output_box = AABB(min_p, max_p);
    return true; // A cube always has a valid bounding box
}

bool Cube::intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const {
    Vector3 ray_origin_at_t0 = ray.origin - m_velocity * ray.time;
    // Transform the ray into object space
    Vector3 object_origin = m_inverse_transform * ray_origin_at_t0; // Use modified origin from time
    Vector3 object_direction = m_inverse_transform.transformDirection(ray.direction);
    Ray object_ray(object_origin, object_direction, ray.time);


    // Perform the Ray-AABB (Slab) intersection test
    // AABB is from P_min = (-1.0, -1.0, -1.0) to P_max = (1.0, 1.0, 1.0)
    
    // initialize t_near to the smallest possible value and t_far to the largest
    double t_near = -1.0 / 0.0; // -Infinity
    double t_far = 1.0 / 0.0;   // +Infinity

    // Check X-Slab
    double t0 = (-1.0 - object_origin.x) / object_direction.x;
    double t1 = (1.0 - object_origin.x) / object_direction.x;
    if (t0 > t1) std::swap(t0, t1);
    t_near = std::max(t0, t_near);
    t_far = std::min(t1, t_far);
    if (t_near > t_far) return false;

    // --- Check Y-Slab ---
    t0 = (-1.0 - object_origin.y) / object_direction.y;
    t1 = (1.0 - object_origin.y) / object_direction.y;
    if (t0 > t1) std::swap(t0, t1);
    t_near = std::max(t0, t_near);
    t_far = std::min(t1, t_far);
    if (t_near > t_far) return false;

    // --- Check Z-Slab ---
    t0 = (-1.0 - object_origin.z) / object_direction.z;
    t1 = (1.0 - object_origin.z) / object_direction.z;
    if (t0 > t1) std::swap(t0, t1);
    t_near = std::max(t0, t_near);
    t_far = std::min(t1, t_far);
    if (t_near > t_far) return false;

    // hit, check if it's in the valid range
    if (t_near < t_min || t_near > t_max) {
        // The first hit is outside the valid range.
        // Check if the second hit (t_far) is valid.
        if (t_far >= t_min && t_far <= t_max) {
             t_near = t_far; // Use the exit point
        } else {
            return false; // Both hits are outside the valid range
        }
    }
    
    // Fill the HitRecord
    rec.t = t_near;
    rec.point = ray.point_at_parameter(rec.t); // Use the original ray
    
    // Find the Normal
    Vector3 object_point = object_ray.point_at_parameter(rec.t);
    Vector3 object_normal;

    // Find the component of the object-space hit point with the largest
    // absolute value. That determines which face was hit.
    double max_c = std::max(std::abs(object_point.x), 
                   std::max(std::abs(object_point.y), std::abs(object_point.z)));
    
    // Use an epsilon for floating point comparison
    const double epsilon = 1e-4;
    
    if (max_c - std::abs(object_point.x) < epsilon) {
        // Hit an X-face
        object_normal = Vector3(object_point.x > 0 ? 1 : -1, 0, 0);
        rec.uv.u = (object_point.z + 1.0) / 2.0;
        rec.uv.v = (object_point.y + 1.0) / 2.0;
    } else if (max_c - std::abs(object_point.y) < epsilon) {
        // Hit a Y-face
        object_normal = Vector3(0, object_point.y > 0 ? 1 : -1, 0);
        rec.uv.u = (object_point.x + 1.0) / 2.0;
        rec.uv.v = (object_point.z + 1.0) / 2.0;
    } else {
        // Hit a Z-face
        object_normal = Vector3(0, 0, object_point.z > 0 ? 1 : -1);
        rec.uv.u = (object_point.x + 1.0) / 2.0;
        rec.uv.v = (object_point.y + 1.0) / 2.0;
    }

    Vector3 outward_normal = (m_inverse_transpose * object_normal).normalize();

    // Transform the normal back to world space
    rec.set_face_normal(ray, outward_normal);

    rec.mat = m_material;

    return true;
}