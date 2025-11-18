//
// Created by alex on 29/10/2025.
//

#include "sphere.h"
#include <limits>
#include <cmath>
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


bool Sphere::getBoundingBox(AABB& output_box) const {
    // Start with invalid bounds
    double infinity = std::numeric_limits<double>::infinity();
    Vector3 min_p(infinity, infinity, infinity);
    Vector3 max_p(-infinity, -infinity, -infinity);

    // Transform the 8 corners of the unit sphere's bounding box (-1 to 1)
    // and find the min/max of the transformed points.
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
    return true; // A sphere always has a valid bounding box
}

bool Sphere::intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const {
    Vector3 ray_origin_at_t0 = ray.origin - m_velocity * ray.time;

    // Transform the ray into object space
    Vector3 object_origin = m_inverse_transform * ray_origin_at_t0;
    Vector3 object_direction = m_inverse_transform.transformDirection(ray.direction);
    Ray object_ray(object_origin, object_direction, ray.time);

    // testing against a unit sphere at (0,0,0)
    // R(t) = object_origin + t * object_direction
    // Sphere: |P|^2 = 1^2
    // |object_origin + t * object_direction|^2 = 1

    // transformation performed on ray not sphere so centre is origin and radius is 1
    Vector3 oc = object_origin; // Center is (0,0,0), so (origin - center) is just origin

    double a = object_direction.dot(object_direction);
    double b = 2.0 * oc.dot(object_direction);
    double c = oc.dot(oc) - 1.0; // radius is 1.0

    double discriminant = b * b - 4 * a * c;

    if (discriminant < 0) {
        return false; // Ray missed
    }

    // Find the nearest root that lies in the acceptable range [t_min, t_max]
    double root = (-b - std::sqrt(discriminant)) / (2.0 * a);
    if (root < t_min || root > t_max) {
        root = (-b + std::sqrt(discriminant)) / (2.0 * a);
        if (root < t_min || root > t_max) {
            return false; // Both roots are outside the valid range
        }
    }

    //valid hit

    // Transform the hit record back to world space

    rec.t = root;

    // Use the original ray to find the world-space hit point
    rec.point = ray.point_at_parameter(rec.t);

    Vector3 object_point = object_ray.point_at_parameter(rec.t);
    Vector3 object_normal = object_point - Vector3(0,0,0); // Normal of unit sphere is just the point
    Vector3 outward_normal = (m_inverse_transpose * object_normal).normalize();

    // Transform the normal using the inverse-transpose matrix
    rec.set_face_normal(ray, outward_normal);

    rec.mat = m_material;

    Vector3 p = object_normal.normalize();
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

    double theta = asin(p.y);
    double phi = atan2(-p.z, p.x) + M_PI;

    rec.uv.u = phi / (2.0 * M_PI);
    rec.uv.v = (theta + M_PI / 2.0) / M_PI;

    // bump mapping
    if (rec.mat.bump_map) {
        // Calculate Tangent (T) and Bitangent (B) in world space
        // Tangent is derivative of position wrt phi (around Y axis)
        // T points along the U direction (latitude line)
        // Standard sphere parameterization derivative: (-sin(phi), 0, -cos(phi)) roughly
        // Or simple cross product: T = cross(Y_axis, N)
        Vector3 Y_axis(0, 1, 0);
        Vector3 N = outward_normal;

        // Handle singularity at poles where N is parallel to Y
        Vector3 T;
        if (std::abs(N.dot(Y_axis)) > 0.999) {
            T = Vector3(1, 0, 0); // Arbitrary for poles
        } else {
            T = Y_axis.cross(N).normalize();
        }
        Vector3 B = N.cross(T).normalize();

        // sample gradients from bump map
        int w = rec.mat.bump_map->getWidth();
        int h = rec.mat.bump_map->getHeight();

        int x = static_cast<int>(rec.uv.u * (w - 1));
        int y = static_cast<int>((1.0 - rec.uv.v) * (h - 1));

        // get brightness
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
        outward_normal = perturbed;
    }


    rec.set_face_normal(ray, outward_normal);
    return true;
}
