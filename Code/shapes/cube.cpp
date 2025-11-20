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

    // Transform the 8 corners of the unit cube's bounding box (-1 to 1)
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

    output_box = AABB(min_p, max_p);
    return true;
}

bool Cube::intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const {
    // Adjust ray for motion blur
    Vector3 ray_origin_at_t0 = ray.origin - m_velocity * ray.time;

    // Transform ray to object space
    Vector3 object_origin = m_inverse_transform * ray_origin_at_t0;
    Vector3 object_direction = m_inverse_transform.transformDirection(ray.direction);

    // Slabs method for AABB intersection against unit cube [-1, 1]
    double t_near = -std::numeric_limits<double>::infinity();
    double t_far = std::numeric_limits<double>::infinity();

    for (int i = 0; i < 3; ++i) {
        double origin = (i == 0) ? object_origin.x : ((i == 1) ? object_origin.y : object_origin.z);
        double dir = (i == 0) ? object_direction.x : ((i == 1) ? object_direction.y : object_direction.z);

        double t0, t1;
        if (dir == 0.0) {
            // Ray is parallel to the slab.
            // If origin is outside [-1, 1], we miss the box entirely.
            if (origin < -1.0 || origin > 1.0) {
                return false;
            }
            // Inside the slab, intersection is everywhere (-inf to +inf)
            t0 = -std::numeric_limits<double>::infinity();
            t1 = std::numeric_limits<double>::infinity();
        } else {
            double inv_d = 1.0 / dir;
            t0 = (-1.0 - origin) * inv_d;
            t1 = (1.0 - origin) * inv_d;
            if (inv_d < 0.0) std::swap(t0, t1);
        }

        if (t0 > t_near) t_near = t0;
        if (t1 < t_far) t_far = t1;

        if (t_near > t_far) return false;
        if (t_far < 0.0) return false;
    }

    // Determine the valid hit distance
    double t_hit = t_near;
    if (t_hit < t_min || t_hit > t_max) {
        t_hit = t_far;
        if (t_hit < t_min || t_hit > t_max) return false;
    }

    rec.t = t_hit;
    rec.point = ray.point_at_parameter(rec.t);
    rec.mat = m_material;

    // Calculate Object Space Point
    Vector3 p = object_origin + object_direction * t_hit;

    // Robust Normal Calculation
    // We determine the face by finding which component (x, y, or z) is closest to +/- 1.0.
    // This avoids state-tracking errors from the Slab loop.
    Vector3 abs_p(std::abs(p.x), std::abs(p.y), std::abs(p.z));
    Vector3 object_normal(0, 0, 0);
    int hit_axis = 0;

    if (abs_p.x >= abs_p.y && abs_p.x >= abs_p.z) {
        hit_axis = 0;
        object_normal.x = (p.x > 0) ? 1.0 : -1.0;
    } else if (abs_p.y >= abs_p.x && abs_p.y >= abs_p.z) {
        hit_axis = 1;
        object_normal.y = (p.y > 0) ? 1.0 : -1.0;
    } else {
        hit_axis = 2;
        object_normal.z = (p.z > 0) ? 1.0 : -1.0;
    }

    // Transform normal to world space
    // m_inverse_transpose is (M^-1)^T, which is the correct matrix for normals.
    Vector3 outward_normal = (m_inverse_transpose * object_normal).normalize();

    // Ensure normal faces against the ray
    rec.set_face_normal(ray, outward_normal);

    // UV Mapping (Simple planar mapping)
    // map the 2 coordinates of the hit face to u,v range [0,1]
    double u = 0.0, v = 0.0;
if (hit_axis == 0) { // X-planes (Left/Right Sides)
        u = (p.y * (object_normal.x > 0 ? -1 : 1) + 1.0) * 0.5; // Map Y to U (Wrap direction)
        v = (p.z + 1.0) * 0.5; // Map Z to V (Height)
    } else if (hit_axis == 1) { // Y-planes (Front/Back Sides)
        u = (p.x * (object_normal.y > 0 ? 1 : -1) + 1.0) * 0.5; // Map X to U
        v = (p.z + 1.0) * 0.5; // Map Z to V (Height)
    } else { // Z-planes (Top/Bottom)
        u = (p.x + 1.0) * 0.5;
        v = (p.y + 1.0) * 0.5;
    }

    double u_offset = 0.0;
    double v_offset = 0.0;
    if (hit_axis == 2) {
        // Z-Axis is TOP/BOTTOM
        if (object_normal.z > 0) {
            u_offset = 1.0; v_offset = 2.0; // Top -> Grid(1,2)
        } else {
            u_offset = 1.0; v_offset = 0.0; // Bottom -> Grid(1,0)
        }
    } else if (hit_axis == 1) {
        // Y-Axis is Front/Back
        if (object_normal.y > 0) {
            u_offset = 1.0; v_offset = 1.0; // Front -> Grid(1,1) (assuming +Y is Front)
        } else {
            u_offset = 3.0; v_offset = 1.0; // Back -> Grid(3,1)
        }
    } else { // hit_axis == 0
        // X-Axis is Left/Right
        if (object_normal.x > 0) {
            u_offset = 2.0; v_offset = 1.0; // Right -> Grid(2,1)
        } else {
            u_offset = 0.0; v_offset = 1.0; // Left -> Grid(0,1)
        }
    }

    rec.uv.u = (u + u_offset) * 0.25;
    rec.uv.v = (v + v_offset) * (1.0/3.0);

    // Bump Mapping
    if (rec.mat.bump_map) {
        // Calculate Tangent (T) and Bitangent (B)
        Vector3 Y_axis(0, 1, 0);
        Vector3 N = outward_normal; // Use the calculated world-space normal

        Vector3 T;
        // Handle singularity at poles (if normal is parallel to Y)
        if (std::abs(N.dot(Y_axis)) > 0.999) {
            T = Vector3(1, 0, 0);
        } else {
            T = Y_axis.cross(N).normalize();
        }
        Vector3 B = N.cross(T).normalize();

        // Sample gradients
        int w = rec.mat.bump_map->getWidth();
        int h = rec.mat.bump_map->getHeight();

        int x = static_cast<int>(rec.uv.u * (w - 1));
        int y = static_cast<int>((1.0 - rec.uv.v) * (h - 1));

        auto get_val = [&](int px, int py) {
            px = std::min(std::max(px, 0), w - 1);
            py = std::min(std::max(py, 0), h - 1);
            Pixel pix = rec.mat.bump_map->getPixel(px, py);
            return (pix.r + pix.g + pix.b) / (3.0 * 255.0);
        };

        double height_c = get_val(x, y);
        double height_u = get_val(x + 1, y);
        double height_v = get_val(x, y + 1);

        double bu = (height_u - height_c) * w;
        double bv = (height_v - height_c) * h;

        double bump_scale = 0.0075;
        Vector3 perturbed = (N + (T * bu + B * bv) * bump_scale).normalize();
        rec.set_face_normal(ray, perturbed);
    }

    return true;
}

bool Cube::any_hit(const Ray& ray, double t_min, double t_max) const {
    Vector3 ray_origin_at_t0 = ray.origin - m_velocity * ray.time;
    Vector3 object_origin = m_inverse_transform * ray_origin_at_t0;
    Vector3 object_direction = m_inverse_transform.transformDirection(ray.direction);

    double t0 = t_min;
    double t1 = t_max;

    double invRayDir = 1.0 / object_direction.x;
    double tNear = (-1.0 - object_origin.x) * invRayDir;
    double tFar  = ( 1.0 - object_origin.x) * invRayDir;
    if (invRayDir < 0.0) std::swap(tNear, tFar);
    t0 = tNear > t0 ? tNear : t0;
    t1 = tFar  < t1 ? tFar  : t1;
    if (t0 > t1) return false;

    invRayDir = 1.0 / object_direction.y;
    tNear = (-1.0 - object_origin.y) * invRayDir;
    tFar  = ( 1.0 - object_origin.y) * invRayDir;
    if (invRayDir < 0.0) std::swap(tNear, tFar);
    t0 = tNear > t0 ? tNear : t0;
    t1 = tFar  < t1 ? tFar  : t1;
    if (t0 > t1) return false;

    invRayDir = 1.0 / object_direction.z;
    tNear = (-1.0 - object_origin.z) * invRayDir;
    tFar  = ( 1.0 - object_origin.z) * invRayDir;
    if (invRayDir < 0.0) std::swap(tNear, tFar);
    t0 = tNear > t0 ? tNear : t0;
    t1 = tFar  < t1 ? tFar  : t1;
    if (t0 > t1) return false;

    return true;
}