//
// Created by alex on 19/11/2025.
//

#include "complex_cube.h"
#include <limits>
#include <cmath>
#include <algorithm>
#include "../Image.h"
#include "../config.h"

ComplexCube::ComplexCube(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat, const Vector3& velocity)
    : Cube(transform, inv_transform, mat, velocity)
{m_max_displacement = Config::Instance().getDouble("advanced.displacement_strength", 0.2);}

bool ComplexCube::getBoundingBox(AABB& output_box) const {
    double expansion = 1.0 + m_max_displacement;
    return getTransformedBoundingBox(output_box, Vector3(-expansion, -expansion, -expansion), Vector3(expansion, expansion, expansion));
}

double ComplexCube::signed_distance_box(const Vector3& p) const {
    Vector3 d = Vector3(std::abs(p.x), std::abs(p.y), std::abs(p.z)) - Vector3(1,1,1);
    double inside_dist = std::min(std::max(d.x, std::max(d.y, d.z)), 0.0);
    double outside_dist = Vector3(std::max(d.x, 0.0), std::max(d.y, 0.0), std::max(d.z, 0.0)).length();
    return inside_dist + outside_dist;
}

void ComplexCube::get_uv_and_normal(const Vector3& p, double& u, double& v, Vector3& normal) const {
    Vector3 abs_p(std::abs(p.x), std::abs(p.y), std::abs(p.z));
    int hit_axis = 0;

    if (abs_p.x >= abs_p.y && abs_p.x >= abs_p.z) {
        hit_axis = 0;
        normal = Vector3((p.x > 0) ? 1.0 : -1.0, 0, 0);
    } else if (abs_p.y >= abs_p.x && abs_p.y >= abs_p.z) {
        hit_axis = 1;
        normal = Vector3(0, (p.y > 0) ? 1.0 : -1.0, 0);
    } else {
        hit_axis = 2;
        normal = Vector3(0, 0, (p.z > 0) ? 1.0 : -1.0);
    }

    double raw_u = 0.0, raw_v = 0.0;
    if (hit_axis == 0) {
        raw_u = (p.y * (normal.x > 0 ? -1 : 1) + 1.0) * 0.5;
        raw_v = (p.z + 1.0) * 0.5;
    } else if (hit_axis == 1) {
        raw_u = (p.x * (normal.y > 0 ? 1 : -1) + 1.0) * 0.5;
        raw_v = (p.z + 1.0) * 0.5;
    } else {
        raw_u = (p.x + 1.0) * 0.5;
        raw_v = (p.y + 1.0) * 0.5;
    }

    raw_u = std::max(0.0, std::min(1.0, raw_u));
    raw_v = std::max(0.0, std::min(1.0, raw_v));


    double u_offset = 0.0;
    double v_offset = 0.0;
    if (hit_axis == 2) {
        if (normal.z > 0) { u_offset = 1.0; v_offset = 2.0; }
        else { u_offset = 1.0; v_offset = 0.0; }
    } else if (hit_axis == 1) {
        if (normal.y > 0) { u_offset = 1.0; v_offset = 1.0; }
        else { u_offset = 3.0; v_offset = 1.0; }
    } else {
        if (normal.x > 0) { u_offset = 2.0; v_offset = 1.0; }
        else { u_offset = 0.0; v_offset = 1.0; }
    }

    u = (raw_u + u_offset) * 0.25;
    v = (raw_v + v_offset) * (1.0/3.0);
}

bool ComplexCube::intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const {
    Vector3 ray_origin_at_t0 = ray.origin - m_velocity * ray.time;
    Vector3 obj_origin = m_inverse_transform * ray_origin_at_t0;
    Vector3 obj_dir = m_inverse_transform.transformDirection(ray.direction);

    double bound = 1.0 + m_max_displacement;
    double t_near = -std::numeric_limits<double>::infinity();
    double t_far = std::numeric_limits<double>::infinity();

    for (int i = 0; i < 3; ++i) {
        double origin = (i == 0) ? obj_origin.x : ((i == 1) ? obj_origin.y : obj_origin.z);
        double dir = (i == 0) ? obj_dir.x : ((i == 1) ? obj_dir.y : obj_dir.z);

        if (dir == 0.0) {
            if (origin < -bound || origin > bound) return false;
        } else {
            double t0 = (-bound - origin) / dir;
            double t1 = (bound - origin) / dir;
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

    for(int i = 0; i < MAX_STEPS; ++i) {
        if (t_current > t_limit) break;

        Vector3 p = obj_origin + obj_dir * t_current;

        double dist_to_base = signed_distance_box(p);

        double u, v;
        Vector3 normal_dummy;
        get_uv_and_normal(p, u, v, normal_dummy);

        double displacement = 0.0;
        if (m_material.bump_map) {
            int w = m_material.bump_map->getWidth();
            int h = m_material.bump_map->getHeight();
            int x = static_cast<int>(u * (w - 1));
            int y = static_cast<int>((1.0 - v) * (h - 1)); // flip v

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
                return signed_distance_box(q) - q_disp;
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

        t_current += std::max(dist_to_surface * 0.6, EPSILON);
    }

    return false;
}
