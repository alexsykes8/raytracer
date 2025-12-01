//
// Created by alex on 21/11/2025.
//

#include "complex_plane.h"
#include <limits>
#include <cmath>
#include <algorithm>
#include "../utilities/Image.h"
#include "../config.h"

// constructor for a complex plane.
ComplexPlane::ComplexPlane(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat, const Vector3& velocity, double shutter_time)
    // initialises member variables with the provided parameters.
    : m_transform(transform), m_inverse_transform(inv_transform), m_material(mat), m_velocity(velocity)
{
    // pre-calculates the inverse transpose matrix, used for transforming normals correctly.
    m_inverse_transpose = m_inverse_transform.transpose();
    // retrieves the maximum displacement value from the configuration, defaulting to 0.2 if not found.
    m_max_displacement = Config::Instance().getDouble("advanced.displacement_strength", 0.2);
}

// calculates the world-space axis-aligned bounding box (aabb) for a given local-space box.
bool ComplexPlane::getTransformedBoundingBox(AABB& output_box, const Vector3& min_p, const Vector3& max_p) const {
    // defines the 8 corners of the local-space bounding box.
    Vector3 corners[8];
    corners[0] = Vector3(min_p.x, min_p.y, min_p.z);
    corners[1] = Vector3(max_p.x, min_p.y, min_p.z);
    corners[2] = Vector3(min_p.x, max_p.y, min_p.z);
    corners[3] = Vector3(max_p.x, max_p.y, min_p.z);
    corners[4] = Vector3(min_p.x, min_p.y, max_p.z);
    corners[5] = Vector3(max_p.x, min_p.y, max_p.z);
    corners[6] = Vector3(min_p.x, max_p.y, max_p.z);
    corners[7] = Vector3(max_p.x, max_p.y, max_p.z);

    // initialises the world-space bounding box extents to infinity.
    Vector3 world_min(std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());
    Vector3 world_max(-std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity());

    // iterates through each corner of the local box.
    for (int i = 0; i < 8; ++i) {
        // transforms the corner from local space to world space.
        // equation: world_pt = m_transform * corners[i]
        Vector3 world_pt = m_transform * corners[i];
        // checks if the object has velocity to account for motion blur.
        if (m_velocity.length() > 1e-6) {
            // for a moving object, the bounding box must encompass its position at the start and end of the frame.
            // calculates the start position (t=0).
            Vector3 p0 = world_pt;
            // calculates the end position (t=1).
            // equation: p1 = world_pt + m_velocity
            Vector3 p1 = world_pt + (m_velocity * m_shutter_time);
            // expands the world-space bounding box to include both start and end positions.
            AABB::updateBounds(p0, world_min, world_max);
            AABB::updateBounds(p1, world_min, world_max);
        } else {
            // for a static object, just expand the bounds to include the transformed corner.
            AABB::updateBounds(world_pt, world_min, world_max);
        }
    }
    // creates the final aabb from the calculated min and max world-space points.
    output_box = AABB(world_min, world_max);
    return true;
}

// calculates the bounding box for the complex plane, accounting for displacement.
bool ComplexPlane::getBoundingBox(AABB& output_box) const {
    // defines the xy bounds of the local-space plane.
    double bound = 1.0;
    // defines the z bound (thickness) of the local-space plane, including max displacement and a small epsilon.
    // equation: z_bound = m_max_displacement + 0.01
    double z_bound = m_max_displacement + 0.01;
    // calls the helper function to get the transformed bounding box.
    return getTransformedBoundingBox(output_box, Vector3(-bound, -bound, -z_bound), Vector3(bound, bound, z_bound));
}

// calculates the signed distance from a point 'p' to the surface of a unit plane with a small thickness.
double ComplexPlane::signed_distance_plane(const Vector3& p) const {
    // defines the half-dimensions of the box representing the thin plane.
    Vector3 b(1.0, 1.0, 0.001);
    // calculates the distance from the point to the box surface along each axis.
    // equation: d = abs(p) - b
    Vector3 d = Vector3(std::abs(p.x), std::abs(p.y), std::abs(p.z)) - b;
    // calculates the distance for a point inside the box (negative value).
    // equation: inside_dist = min(max(d.x, d.y, d.z), 0.0)
    double inside_dist = std::min(std::max(d.x, std::max(d.y, d.z)), 0.0);
    // calculates the distance for a point outside the box (positive value).
    // equation: outside_dist = length(max(d, 0.0))
    double outside_dist = Vector3(std::max(d.x, 0.0), std::max(d.y, 0.0), std::max(d.z, 0.0)).length();
    // returns the exact signed distance by combining inside and outside distances.
    return inside_dist + outside_dist;
}

// calculates the uv coordinates and base normal for a given point on the plane's surface.
void ComplexPlane::get_uv_and_normal(const Vector3& p, double& u, double& v, Vector3& normal) const {
    // the base normal for an untransformed plane is along the z-axis.
    normal = Vector3(0, 0, 1);

    // maps the local x-coordinate from the [-1, 1] range to the [0, 1] u-coordinate.
    // equation: u = (p.x + 1.0) * 0.5
    u = (p.x + 1.0) * 0.5;
    // maps the local y-coordinate from the [-1, 1] range to the [0, 1] v-coordinate.
    // equation: v = (p.y + 1.0) * 0.5
    v = (p.y + 1.0) * 0.5;

    // clamps the uv coordinates to the [0, 1] range to prevent texture wrapping issues.
    u = std::max(0.0, std::min(1.0, u));
    v = std::max(0.0, std::min(1.0, v));
}

// checks for intersection between a ray and the complex plane using ray marching.
bool ComplexPlane::intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const {
    // adjusts the ray origin for motion blur based on the object's velocity and ray time.
    // equation: ray_origin_at_t0 = ray.origin - m_velocity * ray.time
    Vector3 ray_origin_at_t0 = ray.origin - m_velocity * ray.time;
    // transforms the ray origin and direction into the object's local space.
    Vector3 obj_origin = m_inverse_transform * ray_origin_at_t0;
    Vector3 obj_dir = m_inverse_transform.transformDirection(ray.direction);

    // calculates the scaling factor of the ray's direction due to non-uniform scaling.
    double local_ray_scale = obj_dir.length();

    // defines the bounds of the local-space bounding box.
    double xy_bound = 1.0;
    double z_bound = m_max_displacement + 0.01;
    Vector3 bounds(xy_bound, xy_bound, z_bound);

    // initialises near and far intersection distances for the bounding box.
    double t_near = -std::numeric_limits<double>::infinity();
    double t_far = std::numeric_limits<double>::infinity();

    // performs slab testing to find the intersection range with the expanded bounding box.
    for (int i = 0; i < 3; ++i) {
        // gets the origin and direction components for the current axis.
        double origin = (i == 0) ? obj_origin.x : ((i == 1) ? obj_origin.y : obj_origin.z);
        double dir = (i == 0) ? obj_dir.x : ((i == 1) ? obj_dir.y : obj_dir.z);
        double bound_val = (i == 0) ? bounds.x : ((i == 1) ? bounds.y : bounds.z);

        // checks for parallel rays.
        if (dir == 0.0) {
            // if the ray is parallel and outside the slab, it's a miss.
            if (origin < -bound_val || origin > bound_val) return false;
        } else {
            // calculates intersection distances with the slab planes.
            // equation: t0 = (-bound_val - origin) / dir
            double t0 = (-bound_val - origin) / dir;
            // equation: t1 = (bound_val - origin) / dir
            double t1 = (bound_val - origin) / dir;
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
    const double STEP_MULTI = Config::Instance().getDouble("advanced.step_multiplier", 0.8);

    // performs ray marching to find the actual intersection with the displaced surface.
    for(int i = 0; i < MAX_STEPS; ++i) {
        // terminates if the current ray marching step exceeds the limit.
        if (t_current > t_limit) break;

        // calculates the current point in object space along the ray.
        // equation: p = obj_origin + obj_dir * t_current
        Vector3 p = obj_origin + obj_dir * t_current;

        // calculates the signed distance to the base (undisplaced) plane.
        double dist_to_base = signed_distance_plane(p);

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
            int y = static_cast<int>((1.0 - v) * (h - 1));

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
            // 'auto' is a c++ keyword that allows the compiler to deduce the type of a variable from its initialiser.
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
                return signed_distance_plane(q) - q_disp;
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
        // calculates the step size in world space, accounting for non-uniform scaling.
        // equation: step_world = dist_to_surface / local_ray_scale
        double step_world = dist_to_surface / local_ray_scale;
        // advances the ray along its direction, ensuring it doesn't overshoot the surface and takes at least epsilon steps.
        t_current += std::max(step_world * STEP_MULTI, EPSILON);
    }

    return false;
}