#include "complex_sphere.h"
#include <limits>
#include <cmath>
#include <algorithm>
#include "../utilities/Image.h"
#include "../config.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// constructor for a complex sphere.
// initialises the base sphere class and sets the maximum displacement for the bump map.
ComplexSphere::ComplexSphere(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat, const Vector3& velocity)
    : Sphere(transform, inv_transform, mat, velocity) {
    // retrieves the maximum displacement value from the configuration, defaulting to 0.15 if not found.
    m_max_displacement = Config::Instance().getDouble("advanced.displacement_strength", 0.15);
}

// calculates the bounding box for the complex sphere, accounting for displacement.
bool ComplexSphere::getBoundingBox(AABB& output_box) const {
    // calculates the effective radius of the sphere, including maximum displacement.
    // equation: r = 1.0 + m_max_displacement
    double r = 1.0 + m_max_displacement; // the unit sphere has a radius of 1.0, so this expands it.
    // calls the base class method to get the transformed bounding box, using the expanded radius.
    return getTransformedBoundingBox(output_box, Vector3(-r, -r, -r), Vector3(r, r, r));
}

// checks for intersection between a ray and the complex sphere using ray marching.
bool ComplexSphere::intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const {
    // adjusts the ray origin for motion blur based on the object's velocity and ray time.
    Vector3 ray_origin_at_t0 = ray.origin - m_velocity * ray.time;
    // transforms the ray origin and direction into the object's local space.
    Vector3 obj_origin = m_inverse_transform * ray_origin_at_t0;
    Vector3 obj_dir = m_inverse_transform.transformDirection(ray.direction);

    // defines the maximum radius of the sphere, including displacement.
    double max_r = 1.0 + m_max_displacement;
    
    // the ray origin in object space is treated as 'oc' (origin - center, where center is (0,0,0)).
    Vector3 oc = obj_origin; 
    // calculates the 'a' coefficient for the quadratic equation (a*t^2 + b*t + c = 0).
    // equation: a = obj_dir · obj_dir
    double a = obj_dir.dot(obj_dir);
    // calculates the 'b' coefficient for the quadratic equation.
    // equation: b = 2.0 * oc · obj_dir
    double b = 2.0 * oc.dot(obj_dir);
    // calculates the 'c' coefficient for the quadratic equation.
    // equation: c = oc · oc - max_r * max_r
    double c = oc.dot(oc) - max_r * max_r;
    // calculates the discriminant to determine the number of real roots.
    // equation: discriminant = b * b - 4 * a * c
    double discriminant = b * b - 4 * a * c;

    // if the discriminant is negative, the ray missed the bounding sphere.
    if (discriminant < 0) return false;

    // calculates the two possible intersection points (t values) with the bounding sphere.
    // equation: t_entry = (-b - sqrt(discriminant)) / (2.0 * a)
    double t_entry = (-b - std::sqrt(discriminant)) / (2.0 * a);
    // equation: t_exit = (-b + sqrt(discriminant)) / (2.0 * a)
    double t_exit = (-b + std::sqrt(discriminant)) / (2.0 * a);

    // if both intersection points are outside the valid range, the ray missed.
    if (t_exit < t_min || t_entry > t_max) return false;

    // sets the starting and ending points for ray marching within the valid intersection range.
    double t_current = std::max(t_entry, t_min);
    double t_limit = std::min(t_exit, t_max);

    // retrieves ray marching parameters from the configuration.
    const int MAX_STEPS = Config::Instance().getInt("advanced.ray_march_steps", 64);
    const double EPSILON = Config::Instance().getDouble("advanced.epsilon", 0.001);

    // performs ray marching to find the actual intersection with the displaced surface.
    for (int i = 0; i < MAX_STEPS; ++i) {
        // terminates if the current ray marching step exceeds the limit.
        if (t_current > t_limit) break;

        // calculates the current point in object space along the ray.
        Vector3 p = obj_origin + obj_dir * t_current;
        // calculates the distance of the current point from the sphere's origin.
        double dist_from_center = p.length();

        // normalises the point to get its direction from the center.
        Vector3 p_unit = p / dist_from_center;
        
        // gets the uv coordinates for the current point on the unit sphere.
        double u, v;
        get_sphere_uv(p_unit, u, v);

        // calculates displacement based on the bump map, if present.
        double displacement = 0.0;
        if (m_material.bump_map) {
            // gets the pixel colour from the bump map using bilinear interpolation, flipping v.
            Pixel pix = m_material.bump_map->getPixelBilinear(u, 1.0 - v);

            // calculates intensity from the pixel's rgb components.
            // equation: intensity = (pix.r + pix.g + pix.b) / (3.0 * 255.0)
            double intensity = (pix.r + pix.g + pix.b) / (3.0 * 255.0);
            // scales the intensity by the maximum displacement.
            // equation: displacement = intensity * m_max_displacement
            displacement = intensity * m_max_displacement;
        }

        // calculates the signed distance to the displaced surface.
        // equation: dist_to_surface = dist_from_center - (1.0 + displacement)
        double dist_to_surface = dist_from_center - (1.0 + displacement);

        // if the ray is close enough to the surface, a hit is registered.
        if (dist_to_surface < EPSILON) {
            // fills the hit record with intersection details.
            rec.t = t_current;
            // calculates the world-space intersection point.
            rec.point = ray.point_at_parameter(t_current);
            rec.mat = m_material;
            // assigns the uv coordinates to the hit record.
            rec.uv.u = u;
            rec.uv.v = v;

            // 'auto' allows the compiler to deduce the type of a variable from its initialiser.
            // deduces that 'get_sdf' is a lambda function.
            double eps = 0.005;
            auto get_sdf = [&](Vector3 q) {
                // calculates the length of the point from the origin.
                double q_len = q.length();
                // normalises the point to get its direction from the center.
                Vector3 q_unit = q / q_len;
                double qu, qv; 
                get_sphere_uv(q_unit, qu, qv);
                double q_disp = 0.0;
                if (m_material.bump_map) {
                    // gets the pixel colour from the bump map using bilinear interpolation, flipping v.
                    Pixel pix = m_material.bump_map->getPixelBilinear(qu, 1.0 - qv);
                    // scales the intensity by the maximum displacement.
                    // equation: q_disp = ((pix.r + pix.g + pix.b) / (3.0 * 255.0)) * m_max_displacement
                    q_disp = ((pix.r + pix.g + pix.b) / (3.0 * 255.0)) * m_max_displacement;
                }
                // returns the signed distance to the displaced surface at point q.
                // equation: q_len - (1.0 + q_disp)
                return q_len - (1.0 + q_disp);
            };

            // calculates the gradient of the signed distance function to determine the local normal.
            // equation: dx = get_sdf(p + (eps,0,0)) - get_sdf(p - (eps,0,0))
            double dx = get_sdf(p + Vector3(eps, 0, 0)) - get_sdf(p - Vector3(eps, 0, 0));
            // equation: dy = get_sdf(p + (0,eps,0)) - get_sdf(p - (0,eps,0))
            double dy = get_sdf(p + Vector3(0, eps, 0)) - get_sdf(p - Vector3(0, eps, 0));
            // equation: dz = get_sdf(p + (0,0,eps)) - get_sdf(p - (0,0,eps))
            double dz = get_sdf(p + Vector3(0, 0, eps)) - get_sdf(p - Vector3(0, 0, eps));

            // normalises the gradient vector to get the local normal in object space.
            Vector3 local_normal = Vector3(dx, dy, dz).normalize();
            // transforms the local normal to world space using the inverse transpose matrix.
            Vector3 world_normal = (m_inverse_transpose * local_normal).normalize();

            // sets the face normal in the hit record, ensuring it points against the ray.
            rec.set_face_normal(ray, world_normal);
            return true;
        }

        // advances the ray along its direction by a step proportional to the signed distance,
        // ensuring it doesn't overshoot the surface and takes at least EPSILON steps.
        t_current += std::max(dist_to_surface * 0.5, EPSILON);
    }

    return false;
}
