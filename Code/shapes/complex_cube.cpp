//
// Created by alex on 19/11/2025.
//

#include "complex_cube.h"
#include <limits>
#include <cmath>
#include <algorithm>
#include "../utilities/Image.h"
#include "../config.h"

// constructor for a complex cube.
// initialises the base cube class and sets the maximum displacement for the bump map.
// 'const' specifies that a variable's value is constant and tells the compiler to prevent anything from modifying it.
// '&' declares a reference variable. a reference is an alias for an already existing variable.
ComplexCube::ComplexCube(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat, const Vector3& velocity)
    // initialises the base class 'cube' with the provided parameters.
    : Cube(transform, inv_transform, mat, velocity)
{
    // retrieves the maximum displacement value from the configuration, defaulting to 0.2 if not found.
    m_max_displacement = Config::Instance().getDouble("advanced.displacement_strength", 0.2);
}

// calculates the bounding box for the complex cube, accounting for displacement.
bool ComplexCube::getBoundingBox(AABB& output_box) const { // 'const' at the end of a member function declaration means the function will not modify the state of the object it is called on.
    // calculates the expansion factor for the bounding box, including maximum displacement.
    // equation: expansion = 1.0 + m_max_displacement
    double expansion = 1.0 + m_max_displacement; // the unit cube extends from -1 to 1, so this expands it.
    // calls the base class method to get the transformed bounding box, using the expanded size.
    return getTransformedBoundingBox(output_box, Vector3(-expansion, -expansion, -expansion), Vector3(expansion, expansion, expansion));
}

// calculates the signed distance from a point 'p' to the surface of a unit box.
double ComplexCube::signed_distance_box(const Vector3& p) const {
    // calculates the distance from the point to the box surface along each axis.
    // equation: d = abs(p) - (1,1,1)
    Vector3 d = Vector3(std::abs(p.x), std::abs(p.y), std::abs(p.z)) - Vector3(1,1,1);
    // calculates the distance for a point inside the box (negative value).
    // equation: inside_dist = min(max(d.x, d.y, d.z), 0.0)
    double inside_dist = std::min(std::max(d.x, std::max(d.y, d.z)), 0.0);
    // calculates the distance for a point outside the box (positive value).
    // equation: outside_dist = length(max(d, 0.0))
    double outside_dist = Vector3(std::max(d.x, 0.0), std::max(d.y, 0.0), std::max(d.z, 0.0)).length();
    // returns the exact signed distance by combining inside and outside distances.
    return inside_dist + outside_dist;
}

// calculates the uv coordinates and base normal for a given point on the cube's surface.
void ComplexCube::get_uv_and_normal(const Vector3& p, double& u, double& v, Vector3& normal) const {
    // creates a vector with the absolute values of the point's coordinates.
    Vector3 abs_p(std::abs(p.x), std::abs(p.y), std::abs(p.z));
    // initialises the axis of the hit face to 0 (x-axis).
    int hit_axis = 0;

    // determines which face of the cube was hit by finding the largest component of the absolute point.
    if (abs_p.x >= abs_p.y && abs_p.x >= abs_p.z) {
        // hit an x-face.
        hit_axis = 0;
        // sets the normal based on the sign of the x-coordinate.
        normal = Vector3((p.x > 0) ? 1.0 : -1.0, 0, 0);
    } else if (abs_p.y >= abs_p.x && abs_p.y >= abs_p.z) {
        // hit a y-face.
        hit_axis = 1;
        // sets the normal based on the sign of the y-coordinate.
        normal = Vector3(0, (p.y > 0) ? 1.0 : -1.0, 0);
    } else {
        // hit a z-face.
        hit_axis = 2;
        // sets the normal based on the sign of the z-coordinate.
        normal = Vector3(0, 0, (p.z > 0) ? 1.0 : -1.0);
    }

    // initialises raw u and v coordinates.
    double raw_u = 0.0, raw_v = 0.0;
    // calculates raw uv coordinates based on the hit face.
    if (hit_axis == 0) {
        // maps y and z coordinates to uv for x-face.
        raw_u = (p.y * (normal.x > 0 ? -1 : 1) + 1.0) * 0.5;
        raw_v = (p.z + 1.0) * 0.5;
    } else if (hit_axis == 1) {
        // maps x and z coordinates to uv for y-face.
        raw_u = (p.x * (normal.y > 0 ? 1 : -1) + 1.0) * 0.5;
        raw_v = (p.z + 1.0) * 0.5;
    } else {
        // maps x and y coordinates to uv for z-face.
        raw_u = (p.x + 1.0) * 0.5;
        raw_v = (p.y + 1.0) * 0.5;
    }

    // clamps the raw uv coordinates to the [0, 1] range.
    raw_u = std::max(0.0, std::min(1.0, raw_u));
    raw_v = std::max(0.0, std::min(1.0, raw_v));


    // initialises uv offsets for the texture atlas.
    double u_offset = 0.0;
    double v_offset = 0.0;
    // determines the uv offsets based on the hit face to map to the correct region of a 4x3 texture atlas.
    if (hit_axis == 2) {
        // z-axis is top/bottom.
        if (normal.z > 0) { u_offset = 1.0; v_offset = 2.0; }
        else { u_offset = 1.0; v_offset = 0.0; }
    } else if (hit_axis == 1) {
        // y-axis is front/back.
        if (normal.y > 0) { u_offset = 1.0; v_offset = 1.0; }
        else { u_offset = 3.0; v_offset = 1.0; }
    } else {
        // x-axis is left/right.
        if (normal.x > 0) { u_offset = 2.0; v_offset = 1.0; }
        else { u_offset = 0.0; v_offset = 1.0; }
    }

    // calculates the final uv coordinates by applying the offset and scaling for the atlas.
    // equation: u = (raw_u + u_offset) * 0.25
    u = (raw_u + u_offset) * 0.25;
    // equation: v = (raw_v + v_offset) * (1.0/3.0)
    v = (raw_v + v_offset) * (1.0/3.0);
}

// checks for intersection between a ray and the complex cube using ray marching.
bool ComplexCube::intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const {
    // adjusts the ray origin for motion blur based on the object's velocity and ray time.
    // equation: ray_origin_at_t0 = ray.origin - m_velocity * ray.time
    Vector3 ray_origin_at_t0 = ray.origin - m_velocity * ray.time;
    // transforms the ray origin and direction into the object's local space.
    Vector3 obj_origin = m_inverse_transform * ray_origin_at_t0;
    Vector3 obj_dir = m_inverse_transform.transformDirection(ray.direction);

    // defines the maximum bound of the cube, including displacement.
    double bound = 1.0 + m_max_displacement;
    // initialises near and far intersection distances for the bounding box.
    double t_near = -std::numeric_limits<double>::infinity();
    double t_far = std::numeric_limits<double>::infinity();

    // performs slab testing to find the intersection range with the expanded bounding box.
    for (int i = 0; i < 3; ++i) {
        // gets the origin and direction components for the current axis.
        double origin = (i == 0) ? obj_origin.x : ((i == 1) ? obj_origin.y : obj_origin.z);
        double dir = (i == 0) ? obj_dir.x : ((i == 1) ? obj_dir.y : obj_dir.z);

        // checks for parallel rays.
        if (dir == 0.0) {
            // if the ray is parallel and outside the slab, it's a miss.
            if (origin < -bound || origin > bound) return false;
        } else {
            // calculates intersection distances with the slab planes.
            // equation: t0 = (-bound - origin) / dir
            double t0 = (-bound - origin) / dir;
            // equation: t1 = (bound - origin) / dir
            double t1 = (bound - origin) / dir;
            // ensures t0 is the smaller value.
            if (dir < 0.0) std::swap(t0, t1);
            // updates the near and far intersection distances.
            if (t0 > t_near) t_near = t0;
            if (t1 < t_far) t_far = t1;
        }
    }

    // if the intersection range is invalid, the ray missed the bounding box.
    if (t_near > t_far || t_far < 0.0) return false;

    // sets the starting and ending points for ray marching within the valid intersection range.
    double t_current = std::max(t_near, t_min);
    double t_limit = std::min(t_far, t_max);

    // retrieves ray marching parameters from the configuration.
    const int MAX_STEPS = Config::Instance().getInt("advanced.ray_march_steps", 64);
    const double EPSILON = Config::Instance().getDouble("advanced.epsilon", 0.001);

    // performs ray marching to find the actual intersection with the displaced surface.
    for(int i = 0; i < MAX_STEPS; ++i) {
        // terminates if the current ray marching step exceeds the limit.
        if (t_current > t_limit) break;

        // calculates the current point in object space along the ray.
        // equation: p = obj_origin + obj_dir * t_current
        Vector3 p = obj_origin + obj_dir * t_current;

        // calculates the signed distance to the base (undisplaced) cube.
        double dist_to_base = signed_distance_box(p);

        // gets the uv coordinates and a dummy normal for the current point.
        double u, v;
        Vector3 normal_dummy;
        get_uv_and_normal(p, u, v, normal_dummy);

        // calculates displacement based on the bump map, if present.
        double displacement = 0.0;
        if (m_material.bump_map) {
            // gets the dimensions of the bump map.
            int w = m_material.bump_map->getWidth();
            int h = m_material.bump_map->getHeight();
            // converts uv coordinates to pixel coordinates, flipping v.
            int x = static_cast<int>(u * (w - 1));
            int y = static_cast<int>((1.0 - v) * (h - 1)); // flip v

            // clamps pixel coordinates to be within the image bounds.
            x = std::max(0, std::min(x, w-1));
            y = std::max(0, std::min(y, h-1));

            // gets the pixel colour from the bump map.
            Pixel pix = m_material.bump_map->getPixel(x, y);
            // calculates intensity from the pixel's rgb components.
            // equation: intensity = (pix.r + pix.g + pix.b) / (3.0 * 255.0)
            double intensity = (pix.r + pix.g + pix.b) / (3.0 * 255.0);
            // scales the intensity by the maximum displacement.
            // equation: displacement = intensity * m_max_displacement
            displacement = intensity * m_max_displacement;
        }


        // calculates the signed distance to the displaced surface.
        // equation: dist_to_surface = dist_to_base - displacement
        double dist_to_surface = dist_to_base - displacement;

        // if the ray is close enough to the surface, a hit is registered.
        if (dist_to_surface < EPSILON) {
            // fills the hit record with intersection details.
            rec.t = t_current;
            rec.point = ray.point_at_parameter(t_current);
            rec.mat = m_material;

            // defines a small step for calculating the gradient.
            double d = 0.005;
            // 'auto' allows the compiler to deduce the type of a variable from its initialiser.
            // deduces that 'sample_scene' is a lambda function that calculates the signed distance at a given point.
            auto sample_scene = [&](Vector3 q) {
                double qu, qv; Vector3 qn;
                get_uv_and_normal(q, qu, qv, qn);
                double q_disp = 0.0;
                if (m_material.bump_map) {
                    // gets bump map dimensions.
                    int w = m_material.bump_map->getWidth();
                    int h = m_material.bump_map->getHeight();
                    // converts uv to pixel coordinates.
                    int x = static_cast<int>(qu * (w - 1));
                    int y = static_cast<int>((1.0 - qv) * (h - 1));
                    // clamps coordinates.
                     x = std::max(0, std::min(x, w-1));
                     y = std::max(0, std::min(y, h-1));
                    // gets pixel and calculates displacement.
                    Pixel pix = m_material.bump_map->getPixel(x, y);
                    // equation: q_disp = ((pix.r + pix.g + pix.b) / (3.0 * 255.0)) * m_max_displacement
                    q_disp = ((pix.r + pix.g + pix.b) / (3.0 * 255.0)) * m_max_displacement;
                }
                // returns the signed distance to the displaced surface at point q.
                return signed_distance_box(q) - q_disp;
            };

            // calculates the gradient of the signed distance function to determine the local normal.
            // equation: grad_x = sample_scene(p + (d,0,0)) - sample_scene(p - (d,0,0))
            double grad_x = sample_scene(p + Vector3(d,0,0)) - sample_scene(p - Vector3(d,0,0));
            // equation: grad_y = sample_scene(p + (0,d,0)) - sample_scene(p - (0,d,0))
            double grad_y = sample_scene(p + Vector3(0,d,0)) - sample_scene(p - Vector3(0,d,0));
            // equation: grad_z = sample_scene(p + (0,0,d)) - sample_scene(p - (0,0,d))
            double grad_z = sample_scene(p + Vector3(0,0,d)) - sample_scene(p - Vector3(0,0,d));

            // normalises the gradient vector to get the local normal in object space.
            Vector3 local_normal = Vector3(grad_x, grad_y, grad_z).normalize();
            // transforms the local normal to world space using the inverse transpose matrix.
            Vector3 world_normal = (m_inverse_transpose * local_normal).normalize();

            // sets the face normal in the hit record, ensuring it points against the ray.
            rec.set_face_normal(ray, world_normal);
            // assigns the uv coordinates to the hit record.
            rec.uv.u = u;
            rec.uv.v = v;

            return true;
        }

        // advances the ray along its direction by a step proportional to the signed distance,
        // ensuring it doesn't overshoot the surface and takes at least epsilon steps.
        t_current += std::max(dist_to_surface * 0.6, EPSILON);
    }

    return false;
}
