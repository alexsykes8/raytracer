#include "complex_sphere.h"
#include <limits>
#include <cmath>
#include <algorithm>
#include "../Image.h"
#include "../config.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ComplexSphere::ComplexSphere(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat, const Vector3& velocity)
    : Sphere(transform, inv_transform, mat, velocity) {
    m_max_displacement = Config::Instance().getDouble("advanced.displacement_strength", 0.15);
}

bool ComplexSphere::getBoundingBox(AABB& output_box) const {
    double r = 1.0 + m_max_displacement;
    return getTransformedBoundingBox(output_box, Vector3(-r, -r, -r), Vector3(r, r, r));
}


bool ComplexSphere::intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const {
    Vector3 ray_origin_at_t0 = ray.origin - m_velocity * ray.time;
    Vector3 obj_origin = m_inverse_transform * ray_origin_at_t0;
    Vector3 obj_dir = m_inverse_transform.transformDirection(ray.direction);

    double max_r = 1.0 + m_max_displacement;
    
    Vector3 oc = obj_origin; 
    double a = obj_dir.dot(obj_dir);
    double b = 2.0 * oc.dot(obj_dir);
    double c = oc.dot(oc) - max_r * max_r;
    double discriminant = b * b - 4 * a * c;

    if (discriminant < 0) return false;

    double t_entry = (-b - std::sqrt(discriminant)) / (2.0 * a);
    double t_exit = (-b + std::sqrt(discriminant)) / (2.0 * a);

    if (t_exit < t_min || t_entry > t_max) return false;

    double t_current = std::max(t_entry, t_min);
    double t_limit = std::min(t_exit, t_max);

    const int MAX_STEPS = Config::Instance().getInt("advanced.ray_march_steps", 64);
    const double EPSILON = Config::Instance().getDouble("advanced.epsilon", 0.001);

    for (int i = 0; i < MAX_STEPS; ++i) {
        if (t_current > t_limit) break;

        Vector3 p = obj_origin + obj_dir * t_current;
        double dist_from_center = p.length();

        Vector3 p_unit = p / dist_from_center;
        
        double u, v;
        get_sphere_uv(p_unit, u, v);

        double displacement = 0.0;
        if (m_material.bump_map) {
            Pixel pix = m_material.bump_map->getPixelBilinear(u, 1.0 - v);

            double intensity = (pix.r + pix.g + pix.b) / (3.0 * 255.0);
            displacement = intensity * m_max_displacement;
        }

        double dist_to_surface = dist_from_center - (1.0 + displacement);

        if (dist_to_surface < EPSILON) {
            rec.t = t_current;
            rec.point = ray.point_at_parameter(t_current);
            rec.mat = m_material;
            rec.uv.u = u;
            rec.uv.v = v;


            double eps = 0.005;
            auto get_sdf = [&](Vector3 q) {
                double q_len = q.length();
                Vector3 q_unit = q / q_len;
                double qu, qv; 
                get_sphere_uv(q_unit, qu, qv);
                double q_disp = 0.0;
                if (m_material.bump_map) {
                    Pixel pix = m_material.bump_map->getPixelBilinear(qu, 1.0 - qv);
                    q_disp = ((pix.r + pix.g + pix.b) / (3.0 * 255.0)) * m_max_displacement;
                }
                return q_len - (1.0 + q_disp);
            };

            double dx = get_sdf(p + Vector3(eps, 0, 0)) - get_sdf(p - Vector3(eps, 0, 0));
            double dy = get_sdf(p + Vector3(0, eps, 0)) - get_sdf(p - Vector3(0, eps, 0));
            double dz = get_sdf(p + Vector3(0, 0, eps)) - get_sdf(p - Vector3(0, 0, eps));

            Vector3 local_normal = Vector3(dx, dy, dz).normalize();
            Vector3 world_normal = (m_inverse_transpose * local_normal).normalize();

            rec.set_face_normal(ray, world_normal);
            return true;
        }

        t_current += std::max(dist_to_surface * 0.5, EPSILON);
    }

    return false;
}

