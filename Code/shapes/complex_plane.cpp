//
// Created by alex on 21/11/2025.
//

#include "complex_plane.h"
#include <limits>
#include <cmath>
#include <algorithm>
#include "../Image.h"
#include "../config.h"

ComplexPlane::ComplexPlane(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat, const Vector3& velocity)
    : m_transform(transform), m_inverse_transform(inv_transform), m_material(mat), m_velocity(velocity)
{
    m_inverse_transpose = m_inverse_transform.transpose();
    m_max_displacement = Config::Instance().getDouble("advanced.displacement_strength", 0.2);
}

bool ComplexPlane::getTransformedBoundingBox(AABB& output_box, const Vector3& min_p, const Vector3& max_p) const {
    Vector3 corners[8];
    corners[0] = Vector3(min_p.x, min_p.y, min_p.z);
    corners[1] = Vector3(max_p.x, min_p.y, min_p.z);
    corners[2] = Vector3(min_p.x, max_p.y, min_p.z);
    corners[3] = Vector3(max_p.x, max_p.y, min_p.z);
    corners[4] = Vector3(min_p.x, min_p.y, max_p.z);
    corners[5] = Vector3(max_p.x, min_p.y, max_p.z);
    corners[6] = Vector3(min_p.x, max_p.y, max_p.z);
    corners[7] = Vector3(max_p.x, max_p.y, max_p.z);

    Vector3 world_min(std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());
    Vector3 world_max(-std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity());

    for (int i = 0; i < 8; ++i) {
        Vector3 world_pt = m_transform * corners[i];
        if (m_velocity.length() > 1e-6) {
            // Moving object: check start and end positions (t=0 and t=1)
            // Approximate by expanding bounds to include both
            Vector3 p0 = world_pt;
            Vector3 p1 = world_pt + m_velocity;
            AABB::updateBounds(p0, world_min, world_max);
            AABB::updateBounds(p1, world_min, world_max);
        } else {
            AABB::updateBounds(world_pt, world_min, world_max);
        }
    }
    output_box = AABB(world_min, world_max);
    return true;
}

bool ComplexPlane::getBoundingBox(AABB& output_box) const {
    double bound = 1.0;
    double z_bound = m_max_displacement + 0.01; // thickness
    return getTransformedBoundingBox(output_box, Vector3(-bound, -bound, -z_bound), Vector3(bound, bound, z_bound));
}


double ComplexPlane::signed_distance_plane(const Vector3& p) const {
    Vector3 b(1.0, 1.0, 0.001);
    Vector3 d = Vector3(std::abs(p.x), std::abs(p.y), std::abs(p.z)) - b;
    double inside_dist = std::min(std::max(d.x, std::max(d.y, d.z)), 0.0);
    double outside_dist = Vector3(std::max(d.x, 0.0), std::max(d.y, 0.0), std::max(d.z, 0.0)).length();
    return inside_dist + outside_dist;
}

void ComplexPlane::get_uv_and_normal(const Vector3& p, double& u, double& v, Vector3& normal) const {
    normal = Vector3(0, 0, 1);

    // Map [-1, 1] range to [0, 1]
    u = (p.x + 1.0) * 0.5;
    v = (p.y + 1.0) * 0.5;

    // Clamp
    u = std::max(0.0, std::min(1.0, u));
    v = std::max(0.0, std::min(1.0, v));
}


bool ComplexPlane::intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const {
    Vector3 ray_origin_at_t0 = ray.origin - m_velocity * ray.time;
    Vector3 obj_origin = m_inverse_transform * ray_origin_at_t0;
    Vector3 obj_dir = m_inverse_transform.transformDirection(ray.direction);

    double local_ray_scale = obj_dir.length();

    double xy_bound = 1.0;
    double z_bound = m_max_displacement + 0.01;
    Vector3 bounds(xy_bound, xy_bound, z_bound);

    double t_near = -std::numeric_limits<double>::infinity();
    double t_far = std::numeric_limits<double>::infinity();

    for (int i = 0; i < 3; ++i) {
        double origin = (i == 0) ? obj_origin.x : ((i == 1) ? obj_origin.y : obj_origin.z);
        double dir = (i == 0) ? obj_dir.x : ((i == 1) ? obj_dir.y : obj_dir.z);
        double bound_val = (i == 0) ? bounds.x : ((i == 1) ? bounds.y : bounds.z);

        if (dir == 0.0) {
            if (origin < -bound_val || origin > bound_val) return false;
        } else {
            double t0 = (-bound_val - origin) / dir;
            double t1 = (bound_val - origin) / dir;
            if (dir < 0.0) std::swap(t0, t1);
            if (t0 > t_near) t_near = t0;
            if (t1 < t_far) t_far = t1;
        }
    }

    if (t_near > t_far || t_far < 0.0) return false;

    double t_current = std::max(t_near, t_min);
    double t_limit = std::min(t_far, t_max);

    const int MAX_STEPS = Config::Instance().getInt("advanced.ray_march_steps", 64);
    const double EPSILON = Config::Instance().getDouble("advanced.epsilon", 0.001);
    const double STEP_MULTI = Config::Instance().getDouble("advanced.step_multiplier", 0.8);

    for(int i = 0; i < MAX_STEPS; ++i) {
        if (t_current > t_limit) break;

        Vector3 p = obj_origin + obj_dir * t_current;

        double dist_to_base = signed_distance_plane(p);

        double u, v;
        Vector3 normal_dummy;
        get_uv_and_normal(p, u, v, normal_dummy);

        double displacement = 0.0;
        if (m_material.bump_map) {
            int w = m_material.bump_map->getWidth();
            int h = m_material.bump_map->getHeight();
            int x = static_cast<int>(u * (w - 1));
            int y = static_cast<int>((1.0 - v) * (h - 1)); // Flip V

            x = std::max(0, std::min(x, w-1));
            y = std::max(0, std::min(y, h-1));

            Pixel pix = m_material.bump_map->getPixel(x, y);
            double intensity = (pix.r + pix.g + pix.b) / (3.0 * 255.0);
            displacement = intensity * m_max_displacement;
        }

        double dist_to_surface = dist_to_base - displacement;

        if (dist_to_surface < EPSILON) {
            rec.t = t_current;
            rec.point = ray.point_at_parameter(t_current);
            rec.mat = m_material;

            double d = 0.005;
            auto sample_scene = [&](Vector3 q) {
                double qu, qv; Vector3 qn;
                get_uv_and_normal(q, qu, qv, qn);
                double q_disp = 0.0;
                if (m_material.bump_map) {
                    int w = m_material.bump_map->getWidth();
                    int h = m_material.bump_map->getHeight();
                    int x = static_cast<int>(qu * (w - 1));
                    int y = static_cast<int>((1.0 - qv) * (h - 1));
                     x = std::max(0, std::min(x, w-1));
                     y = std::max(0, std::min(y, h-1));
                    Pixel pix = m_material.bump_map->getPixel(x, y);
                    q_disp = ((pix.r + pix.g + pix.b) / (3.0 * 255.0)) * m_max_displacement;
                }
                return signed_distance_plane(q) - q_disp;
            };

            double grad_x = sample_scene(p + Vector3(d,0,0)) - sample_scene(p - Vector3(d,0,0));
            double grad_y = sample_scene(p + Vector3(0,d,0)) - sample_scene(p - Vector3(0,d,0));
            double grad_z = sample_scene(p + Vector3(0,0,d)) - sample_scene(p - Vector3(0,0,d));

            Vector3 local_normal = Vector3(grad_x, grad_y, grad_z).normalize();
            Vector3 world_normal = (m_inverse_transpose * local_normal).normalize();

            rec.set_face_normal(ray, world_normal);
            rec.uv.u = u;
            rec.uv.v = v;

            return true;
        }
        double step_world = dist_to_surface / local_ray_scale;
        t_current += std::max(step_world * STEP_MULTI, EPSILON);
    }

    return false;
}