#include "../shapes/plane.h"
#include <limits>
#include "material.h"
#include "../utilities/Image.h"
#include "../config.h"


// calculates the axis-aligned bounding box (aabb) for the plane.
bool Plane::getBoundingBox(AABB& output_box) const {
    // calculates the four world-space vertices of the quad from the pre-calculated triangle data.
    // corner c0.
    Vector3 v0 = m_t1_v0;          // Corner c0
    // corner c1.
    // equation: v1 = m_t1_v0 + m_t1_edge1
    Vector3 v1 = m_t1_v0 + m_t1_edge1; // Corner c1
    // corner c2.
    // equation: v2 = m_t1_v0 + m_t1_edge2
    Vector3 v2 = m_t1_v0 + m_t1_edge2; // Corner c2
    // corner c3.
    // equation: v3 = m_t2_v0 + m_t2_edge1
    Vector3 v3 = m_t2_v0 + m_t2_edge1; // Corner c3 (calculated as c1 + edge_c3_c1)

    // initialises the bounding box extents with the first vertex.
    Vector3 min_p = v0;
    Vector3 max_p = v0;

    // expands the bounding box to include the other three vertices.
    AABB::updateBounds(v1, min_p, max_p);
    AABB::updateBounds(v2, min_p, max_p);
    AABB::updateBounds(v3, min_p, max_p);

    // adds a small epsilon to give the flat plane some thickness, preventing issues with axis-aligned rays.
    const double epsilon = 1e-4;
    min_p.x -= epsilon; max_p.x += epsilon;
    min_p.y -= epsilon; max_p.y += epsilon;
    min_p.z -= epsilon; max_p.z += epsilon;

    // creates the final aabb from the calculated min and max points.
    output_box = AABB(min_p, max_p);
    return true;
}

// constructor for a plane, defined by four corner vertices.
Plane::Plane(const Vector3& c0, const Vector3& c1, const Vector3& c2, const Vector3& c3, const Material& mat, const Vector3& velocity) : m_material(mat), m_velocity(velocity)
{
    // pre-calculates data for the first triangle (c0, c1, c2) to optimise intersection tests.
    // sets the origin vertex of the first triangle.
    m_t1_v0 = c0;
    // calculates the first edge vector of the triangle.
    // equation: edge1 = c1 - c0
    m_t1_edge1 = c1 - c0;
    // calculates the second edge vector of the triangle.
    // equation: edge2 = c2 - c0
    m_t1_edge2 = c2 - c0;

    // pre-calculates data for the second triangle (c1, c3, c2).
    // sets the origin vertex of the second triangle.
    m_t2_v0 = c1;
    // calculates the first edge vector of the second triangle.
    // equation: edge1 = c3 - c1
    m_t2_edge1 = c3 - c1;
    // calculates the second edge vector of the second triangle.
    // equation: edge2 = c2 - c1
    m_t2_edge2 = c2 - c1;

    // calculates the plane's normal vector using the cross product of the first triangle's edges.
    // equation: normal = (edge1 x edge2) normalized
    m_normal = m_t1_edge1.cross(m_t1_edge2).normalize();
}

// implements the möller-trumbore ray-triangle intersection algorithm.
bool Plane::rayTriangleIntersect(
    const Ray& ray, double t_min, double t_max,
    const Vector3& v0, const Vector3& edge1, const Vector3& edge2,
    double& out_t, double& out_u, double& out_v
) const {
    // retrieves a small epsilon value from configuration for floating-point comparisons.
    double EPSILON = Config::Instance().getDouble("advanced.epsilon", 0.001);
    // calculates a vector perpendicular to the ray direction and the second edge.
    // equation: h = ray.direction x edge2
    Vector3 h = ray.direction.cross(edge2);
    // calculates the dot product of the first edge and the perpendicular vector.
    // this is proportional to the cosine of the angle between the ray and the triangle plane.
    // equation: a = edge1 · h
    double a = edge1.dot(h);

    // if 'a' is close to zero, the ray is parallel to the triangle plane.
    if (a > -EPSILON && a < EPSILON)
        return false; // Ray is parallel to this triangle.

    // calculates the inverse of 'a' to avoid future divisions.
    // equation: f = 1.0 / a
    double f = 1.0 / a;
    // calculates the vector from the triangle's origin vertex to the ray's origin.
    // equation: s = ray.origin - v0
    Vector3 s = ray.origin - v0;
    // calculates the barycentric u-coordinate.
    // equation: u = f * (s · h)
    double u = f * s.dot(h);

    // if u is outside [0, 1], the intersection point is outside the triangle.
    if (u < 0.0 || u > 1.0)
        return false;

    // calculates a vector for the v-coordinate calculation.
    // equation: q = s x edge1
    Vector3 q = s.cross(edge1);
    // calculates the barycentric v-coordinate.
    // equation: v = f * (ray.direction · q)
    double v = f * ray.direction.dot(q);

    // if v is negative or u+v is greater than 1, the intersection point is outside the triangle.
    if (v < 0.0 || u + v > 1.0)
        return false;

    // computes the distance 't' along the ray to the intersection point.
    // equation: t = f * (edge2 · q)
    out_t = f * edge2.dot(q);

    // checks if the intersection distance is within the valid range [t_min, t_max].
    if (out_t > t_min && out_t < t_max) {
        // stores the barycentric coordinates if the hit is valid.
        out_u = u;
        out_v = v;
        return true;
    }

    return false;
}

// checks for an intersection between a ray and the plane (which is composed of two triangles).
bool Plane::intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const {
    // adjusts the ray origin for motion blur based on the object's velocity and ray time.
    // equation: ray_origin_at_t0 = ray.origin - m_velocity * ray.time
    Vector3 ray_origin_at_t0 = ray.origin - m_velocity * ray.time;
    // creates a new ray at time t=0 for the intersection test.
    Ray ray_at_t0(ray_origin_at_t0, ray.direction, ray.time);

    // variables to store intersection results for each triangle.
    double t1, u1, v1;
    double t2, u2, v2;

    // checks for intersection with the first triangle of the quad.
    bool hit1 = rayTriangleIntersect(ray_at_t0, t_min, t_max, m_t1_v0, m_t1_edge1, m_t1_edge2, t1, u1, v1);
    // checks for intersection with the second triangle of the quad.
    bool hit2 = rayTriangleIntersect(ray_at_t0, t_min, t_max, m_t2_v0, m_t2_edge1, m_t2_edge2, t2, u2, v2);

    // if neither triangle was hit, there is no intersection.
    if (!hit1 && !hit2) {
        return false; // Missed both
    }

    // determines which hit is closer (if both were hit) or which one was hit.
    double t_hit;
    bool hit_triangle_1;

    if (hit1 && hit2) {
        // if both triangles were hit, choose the one with the smaller 't' value (the closer one).
        if (t1 < t2) {  // hit triangle 1 first
            t_hit = t1;
            hit_triangle_1 = true;
        } else {
            t_hit = t2;     // hit triangle 2 first
            hit_triangle_1 = false;
        }
    } else if (hit1) {  // only triangle 1 was hit.
        t_hit = t1;
        hit_triangle_1 = true;
    } else { // only triangle 2 was hit.
        t_hit = t2;     // hit just triangle 2
        hit_triangle_1 = false;
    }

    // populates the hit record with intersection details.
    rec.t = t_hit;
    // calculates the world-space intersection point using the original ray.
    // equation: point = ray.origin + ray.direction * t_hit
    rec.point = ray.point_at_parameter(t_hit);

    // sets the face normal, ensuring it points against the ray.
    rec.set_face_normal(ray, m_normal);
    Vector3 outward_normal = m_normal;
    rec.mat = m_material;

    // converts the barycentric coordinates (u, v) of the hit triangle to standard [0,1] uv coordinates for the quad.
    if (hit_triangle_1) {
        rec.uv.u = u1;
        rec.uv.v = v1;
    } else {
        rec.uv.u = 1.0 - v2;
        rec.uv.v = u2 + v2;
    }
    // applies bump mapping if a bump map is present in the material.
    if (rec.mat.bump_map) {
        // for a plane, the tangent (t) and bitangent (b) align with the edges used to define the uvs.
        Vector3 T = m_t1_edge1.normalize();
        Vector3 B = m_t1_edge2.normalize();
        Vector3 N = outward_normal;

        // gets the dimensions of the bump map.
        int w = rec.mat.bump_map->getWidth();
        int h = rec.mat.bump_map->getHeight();

        // converts uv coordinates to pixel coordinates, flipping v.
        int x = static_cast<int>(rec.uv.u * (w - 1));
        int y = static_cast<int>((1.0 - rec.uv.v) * (h - 1)); // Flip V for image coords

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
