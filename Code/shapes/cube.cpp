//
// Created by alex on 29/10/2025.
//

#include "cube.h"
#include <limits>
#include <cmath>
#include <algorithm>
#include "../utilities/Image.h"

// constructor for a cube.
Cube::Cube(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat, const Vector3& velocity)
    // initialises the base class 'transformedshape' with the provided parameters.
    : TransformedShape(transform, inv_transform, mat, velocity)
{}

// calculates the axis-aligned bounding box (aabb) for the transformed cube.
bool Cube::getBoundingBox(AABB& output_box) const { // 'const' at the end of a member function declaration means the function will not modify the state of the object it is called on.
    // defines the local-space unit cube bounds from (-1,-1,-1) to (1,1,1).
    // calls the base class helper to transform these bounds into a world-space aabb.
    return getTransformedBoundingBox(output_box, Vector3(-1, -1, -1), Vector3(1, 1, 1));
}

// checks for intersection between a ray and the cube.
bool Cube::intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const {
    // adjusts the ray origin for motion blur based on the object's velocity and ray time.
    // equation: ray_origin_at_t0 = ray.origin - m_velocity * ray.time
    Vector3 ray_origin_at_t0 = ray.origin - m_velocity * ray.time;

    // transforms the ray from world space to the cube's local object space.
    // equation: object_origin = m_inverse_transform * ray_origin_at_t0
    Vector3 object_origin = m_inverse_transform * ray_origin_at_t0;
    // equation: object_direction = m_inverse_transform.transformdirection(ray.direction)
    Vector3 object_direction = m_inverse_transform.transformDirection(ray.direction);

    // uses the slab testing method for ray-aabb intersection against the local unit cube [-1, 1].
    // initialises the near and far intersection distances to represent an infinite range.
    double t_near = -std::numeric_limits<double>::infinity();
    double t_far = std::numeric_limits<double>::infinity();

    // iterates through the x, y, and z axes.
    for (int i = 0; i < 3; ++i) {
        // gets the origin and direction components for the current axis.
        double origin = (i == 0) ? object_origin.x : ((i == 1) ? object_origin.y : object_origin.z);
        double dir = (i == 0) ? object_direction.x : ((i == 1) ? object_direction.y : object_direction.z);

        double t0, t1;
        if (dir == 0.0) {
            // Ray is parallel to the slab.
            // If origin is outside [-1, 1], the box is missed entirely.
            // if the ray is parallel to a slab and its origin is outside the slab, it can never intersect.
            if (origin < -1.0 || origin > 1.0) {
                return false;
            }
            // if inside, the intersection range for this slab is infinite; do nothing to t_near/t_far.
            t0 = -std::numeric_limits<double>::infinity();
            t1 = std::numeric_limits<double>::infinity();
        } else {
            // calculates the inverse of the direction component to avoid multiple divisions.
            // equation: inv_d = 1.0 / dir
            double inv_d = 1.0 / dir;
            // calculates the intersection distances with the two planes of the slab.
            // equation: t0 = (-1.0 - origin) * inv_d
            t0 = (-1.0 - origin) * inv_d;
            // equation: t1 = (1.0 - origin) * inv_d
            t1 = (1.0 - origin) * inv_d;
            // ensures t0 is the smaller (entry) distance and t1 is the larger (exit) distance.
            if (inv_d < 0.0) std::swap(t0, t1);
        }

        // updates the overall intersection range by taking the maximum of the near distances and the minimum of the far distances.
        if (t0 > t_near) t_near = t0;
        if (t1 < t_far) t_far = t1;

        // if the intersection range becomes invalid (near > far), the ray has missed the cube.
        if (t_near > t_far) return false;
        // if the entire intersection range is behind the ray's origin, it's a miss.
        if (t_far < 0.0) return false;
    }

    // determines the valid hit distance from the calculated intersection range.
    // the first potential hit is at the nearest intersection point.
    double t_hit = t_near;
    // if the nearest hit is outside the valid range [t_min, t_max], check the farthest hit.
    if (t_hit < t_min || t_hit > t_max) {
        t_hit = t_far;
        // if the farthest hit is also outside the valid range, there is no valid intersection.
        if (t_hit < t_min || t_hit > t_max) return false;
    }

    // populates the hit record with intersection details.
    rec.t = t_hit;
    // calculates the world-space intersection point using the original ray.
    // equation: point = ray.origin + ray.direction * t_hit
    rec.point = ray.point_at_parameter(rec.t);
    rec.mat = m_material;

    // calculates the intersection point in the cube's local object space.
    // equation: p = object_origin + object_direction * t_hit
    Vector3 p = object_origin + object_direction * t_hit;

    // determines which face was hit by finding the largest component of the local hit point.
    Vector3 abs_p(std::abs(p.x), std::abs(p.y), std::abs(p.z));
    Vector3 object_normal(0, 0, 0);
    int hit_axis = 0;

    // if the x-component is the largest, it's an x-face.
    if (abs_p.x >= abs_p.y && abs_p.x >= abs_p.z) {
        hit_axis = 0;
        // sets the local normal based on the sign of the x-coordinate.
        object_normal.x = (p.x > 0) ? 1.0 : -1.0;
    // if the y-component is the largest, it's a y-face.
    } else if (abs_p.y >= abs_p.x && abs_p.y >= abs_p.z) {
        hit_axis = 1;
        // sets the local normal based on the sign of the y-coordinate.
        object_normal.y = (p.y > 0) ? 1.0 : -1.0;
    } else {
        // otherwise, it's a z-face.
        hit_axis = 2;
        // sets the local normal based on the sign of the z-coordinate.
        object_normal.z = (p.z > 0) ? 1.0 : -1.0;
    }

    // transforms the local normal to world space using the inverse transpose matrix.
    Vector3 outward_normal = (m_inverse_transpose * object_normal).normalize();

    // sets the face normal in the hit record, ensuring it points against the ray.
    rec.set_face_normal(ray, outward_normal);

    // calculates raw uv coordinates based on the hit face.
    double u = 0.0, v = 0.0;
    if (hit_axis == 0) { // x-planes (left/right sides)
        // maps y and z coordinates to uv for x-face.
        u = (p.y * (object_normal.x > 0 ? -1 : 1) + 1.0) * 0.5;
        v = (p.z + 1.0) * 0.5;
    } else if (hit_axis == 1) { // y-planes (front/back sides)
        // maps x and z coordinates to uv for y-face.
        u = (p.x * (object_normal.y > 0 ? 1 : -1) + 1.0) * 0.5;
        v = (p.z + 1.0) * 0.5;
    } else { // z-planes (top/bottom)
        // maps x and y coordinates to uv for z-face.
        u = (p.x + 1.0) * 0.5;
        v = (p.y + 1.0) * 0.5;
    }

    double u_offset = 0.0;
    double v_offset = 0.0;
    if (hit_axis == 2) {
        // z-axis is top/bottom.
        if (object_normal.z > 0) {
            u_offset = 1.0; v_offset = 2.0; // top -> grid(1,2)
        } else {
            u_offset = 1.0; v_offset = 0.0; // bottom -> grid(1,0)
        }
    } else if (hit_axis == 1) {
        // y-axis is front/back.
        if (object_normal.y > 0) {
            u_offset = 1.0; v_offset = 1.0; // front -> grid(1,1)
        } else {
            u_offset = 3.0; v_offset = 1.0; // back -> grid(3,1)
        }
    } else {
        // x-axis is left/right.
        if (object_normal.x > 0) {
            u_offset = 2.0; v_offset = 1.0; // right -> grid(2,1)
        } else {
            u_offset = 0.0; v_offset = 1.0; // left -> grid(0,1)
        }
    }

    // calculates the final uv coordinates by applying the offset and scaling for the atlas.
    // equation: u = (u + u_offset) * 0.25
    rec.uv.u = (u + u_offset) * 0.25;
    // equation: v = (v + v_offset) * (1.0/3.0)
    rec.uv.v = (v + v_offset) * (1.0/3.0);

    // applies bump mapping if a bump map is present in the material.
    if (rec.mat.bump_map) {
        // calculates tangent (t) and bitangent (b) vectors to form the tangent space.
        Vector3 Y_axis(0, 1, 0);
        Vector3 N = outward_normal; // use the calculated world-space normal.

        Vector3 T;
        // handles the case where the normal is parallel to the up-vector (y-axis) to avoid a zero-length cross product.
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

        // 'auto' allows the compiler to deduce the type of a variable from its initialiser.
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
        rec.set_face_normal(ray, perturbed);
    }

    return true;
}
