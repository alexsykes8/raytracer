#include "complex_sphere.h"
#include <limits>
#include <cmath>
#include <algorithm>
#include "../Image.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void updateBounds(const Vector3& p, Vector3& min_p, Vector3& max_p) {
    min_p.x = std::min(min_p.x, p.x);
    min_p.y = std::min(min_p.y, p.y);
    min_p.z = std::min(min_p.z, p.z);
    max_p.x = std::max(max_p.x, p.x);
    max_p.y = std::max(max_p.y, p.y);
    max_p.z = std::max(max_p.z, p.z);
}

bool ComplexSphere::getBoundingBox(AABB& output_box) const {
    double infinity = std::numeric_limits<double>::infinity();
    Vector3 min_p(infinity, infinity, infinity);
    Vector3 max_p(-infinity, -infinity, -infinity);

    double r = 1.0 + MAX_DISPLACEMENT;

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            for (int k = 0; k < 2; ++k) {
                Vector3 corner(
                    (i == 0) ? -r : r,
                    (j == 0) ? -r : r,
                    (k == 0) ? -r : r
                );
                Vector3 transformed_corner = m_transform * corner;
                updateBounds(transformed_corner, min_p, max_p);
            }
        }
    }
    output_box = AABB(min_p, max_p);
    return true;
}

void ComplexSphere::get_sphere_uv(const Vector3& p, double& u, double& v) const {

    double theta = asin(p.y);
    double phi = atan2(-p.z, p.x) + M_PI;
    u = phi / (2.0 * M_PI);
    v = (theta + M_PI / 2.0) / M_PI;
}

bool ComplexSphere::intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const {
    Vector3 ray_origin_at_t0 = ray.origin - m_velocity * ray.time;
    Vector3 obj_origin = m_inverse_transform * ray_origin_at_t0;
    Vector3 obj_dir = m_inverse_transform.transformDirection(ray.direction);

    double max_r = 1.0 + MAX_DISPLACEMENT;
    
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

    const int MAX_STEPS = 128;
    const double EPSILON = 0.001;

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
            displacement = intensity * MAX_DISPLACEMENT;
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
                    q_disp = ((pix.r + pix.g + pix.b) / (3.0 * 255.0)) * MAX_DISPLACEMENT;
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