//
// Created by alex on 29/10/2025.
//

#include "sphere.h"
#include <limits>
#include <cmath>
#include "../utilities/Image.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// constructor for a sphere.
Sphere::Sphere(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat, const Vector3& velocity)
    // initialises the base class 'transformedshape' with the provided parameters.
    : TransformedShape(transform, inv_transform, mat, velocity)
{}

// calculates the axis-aligned bounding box (aabb) for the transformed sphere.
bool Sphere::getBoundingBox(AABB& output_box) const {
    // defines the local-space unit sphere bounds from (-1,-1,-1) to (1,1,1).
    // calls the base class helper to transform these bounds into a world-space aabb.
    return getTransformedBoundingBox(output_box, Vector3(-1, -1, -1), Vector3(1, 1, 1));
}

// calculates the spherical texture coordinates (u, v) for a point 'p' on a unit sphere.
void Sphere::get_sphere_uv(const Vector3& p, double& u, double& v) {
    // calculates the latitude (theta) from the y-coordinate.
    // equation: θ = asin(p.y)
    double theta = std::asin(p.y);
    // calculates the longitude (phi) from the x and z coordinates.
    // equation: φ = atan2(-p.z, p.x) + π
    double phi = std::atan2(-p.z, p.x) + M_PI;
    // maps phi from [0, 2π] to the u-coordinate range [0, 1].
    // equation: u = φ / (2 * π)
    u = phi / (2.0 * M_PI);
    // maps theta from [-π/2, π/2] to the v-coordinate range [0, 1].
    // equation: v = (θ + π/2) / π
    v = (theta + M_PI / 2.0) / M_PI;
}

// checks for intersection between a ray and the sphere.
bool Sphere::intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const {
    // adjusts the ray origin for motion blur based on the object's velocity and ray time.
    // equation: ray_origin_at_t0 = ray.origin - m_velocity * ray.time
    Vector3 ray_origin_at_t0 = ray.origin - m_velocity * ray.time;

    // transforms the ray from world space to the sphere's local object space.
    // equation: object_origin = m_inverse_transform * ray_origin_at_t0
    Vector3 object_origin = m_inverse_transform * ray_origin_at_t0;
    // equation: object_direction = m_inverse_transform.transformdirection(ray.direction)
    Vector3 object_direction = m_inverse_transform.transformDirection(ray.direction);
    // creates a new ray in the local object space.
    Ray object_ray(object_origin, object_direction, ray.time);

    // solves the quadratic equation for ray-sphere intersection (a*t^2 + 2*b*t + c = 0).
    // the sphere is a unit sphere at the origin in its local space.
    // the vector from the sphere's center (0,0,0) to the ray's origin.
    Vector3 oc = object_origin;

    // calculates the 'a' coefficient of the quadratic equation.
    // equation: a = object_direction · object_direction
    double a = object_direction.dot(object_direction);
    // calculates the 'b' coefficient of the quadratic equation.
    // equation: b = 2.0 * (oc · object_direction)
    double b = 2.0 * oc.dot(object_direction);
    // calculates the 'c' coefficient of the quadratic equation.
    // equation: c = (oc · oc) - r^2, where r=1.
    double c = oc.dot(oc) - 1.0;

    // calculates the discriminant to determine the number of real roots (intersections).
    // equation: discriminant = b^2 - 4ac
    double discriminant = b * b - 4 * a * c;

    if (discriminant < 0) {
        return false; // ray missed the sphere.
    }

    // finds the nearest valid intersection point (root) within the acceptable range [t_min, t_max].
    // equation: root = (-b - sqrt(discriminant)) / (2a)
    double root = (-b - std::sqrt(discriminant)) / (2.0 * a);
    if (root < t_min || root > t_max) {
        // if the first root is invalid, check the second root.
        // equation: root = (-b + sqrt(discriminant)) / (2a)
        root = (-b + std::sqrt(discriminant)) / (2.0 * a);
        if (root < t_min || root > t_max) {
            return false; // both roots are outside the valid range.
        }
    }

    // populates the hit record with intersection details.
    rec.t = root;

    // calculates the world-space intersection point using the original ray.
    // equation: point = ray.origin + ray.direction * t
    rec.point = ray.point_at_parameter(rec.t);

    // calculates the intersection point in the sphere's local object space.
    Vector3 object_point = object_ray.point_at_parameter(rec.t);
    // for a unit sphere at the origin, the normal is the point on the surface.
    Vector3 object_normal = object_point - Vector3(0,0,0);
    // transforms the local normal to world space using the inverse transpose matrix.
    Vector3 outward_normal = (m_inverse_transpose * object_normal).normalize();

    // sets the face normal in the hit record, ensuring it points against the ray.
    rec.set_face_normal(ray, outward_normal);

    rec.mat = m_material;

    // calculates the uv coordinates for the hit point.
    Vector3 p_unit = object_normal.normalize();
    get_sphere_uv(p_unit, rec.uv.u, rec.uv.v);

    // applies bump mapping if a bump map is present in the material.
    if (rec.mat.bump_map) {
        // calculates tangent (t) and bitangent (b) vectors to form the tangent space.
        Vector3 Y_axis(0, 1, 0);
        Vector3 N = outward_normal;

        // handles the case where the normal is parallel to the up-vector (y-axis) to avoid a zero-length cross product.
        Vector3 T;
        if (std::abs(N.dot(Y_axis)) > 0.999) {
            // if at a "pole", use an arbitrary tangent.
            T = Vector3(1, 0, 0);
        } else {
            // otherwise, calculate the tangent as perpendicular to the normal and the up-vector.
            T = Y_axis.cross(N).normalize();
        }
        // calculates the bitangent as perpendicular to the normal and tangent.
        Vector3 B = N.cross(T).normalize();

        // samples gradients from the bump map.
        int w = rec.mat.bump_map->getWidth();
        int h = rec.mat.bump_map->getHeight();

        // converts uv coordinates to pixel coordinates, flipping v.
        int x = static_cast<int>(rec.uv.u * (w - 1));
        int y = static_cast<int>((1.0 - rec.uv.v) * (h - 1));

        // 'auto' means that the compiler deduces the type of a variable from its initialiser.
        // deduces that 'get_val' is a lambda function that samples the bump map intensity at a given pixel.
        auto get_val = [&](int px, int py) {
            // clamps pixel coordinates to be within the image bounds.
            px = std::min(std::max(px, 0), w - 1);
            py = std::min(std::max(py, 0), h - 1);
            // gets the pixel colour from the bump map.
            Pixel pix = rec.mat.bump_map->getPixel(px, py);
            // calculates intensity from the pixel's rgb components.
            // equation: intensity = (pix.r + pix.g + pix.b) / (3.0 * 255.0)
            return (pix.r + pix.g + pix.b) / (3.0 * 255.0);
        };

        // samples the height at the current point and its neighbours in u and v directions.
        double height_c = get_val(x, y);
        double height_u = get_val(x + 1, y);
        double height_v = get_val(x, y + 1);

        // calculates the gradients in the u and v directions (finite differences).
        double bu = (height_u - height_c) * w;
        double bv = (height_v - height_c) * h;

        // perturbs the original normal using the gradients and tangent space vectors.
        double bump_scale = 0.0075;
        Vector3 perturbed = (N + (T * bu + B * bv) * bump_scale).normalize();
        outward_normal = perturbed;
    }

    // sets the final (potentially perturbed) normal in the hit record.
    rec.set_face_normal(ray, outward_normal);
    return true;
}
