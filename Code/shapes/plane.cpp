#include "../shapes/plane.h"
#include <limits>
#include "../material.h"

static void updateBounds(const Vector3& p, Vector3& min_p, Vector3& max_p) {
    min_p.x = std::min(min_p.x, p.x);
    min_p.y = std::min(min_p.y, p.y);
    min_p.z = std::min(min_p.z, p.z);
    max_p.x = std::max(max_p.x, p.x);
    max_p.y = std::max(max_p.y, p.y);
    max_p.z = std::max(max_p.z, p.z);
}


bool Plane::getBoundingBox(AABB& output_box) const {
    // Calculate the actual world-space vertices of the quad
    // based on the stored triangle data
    Vector3 v0 = m_t1_v0;          // Corner c0
    Vector3 v1 = m_t1_v0 + m_t1_edge1; // Corner c1
    Vector3 v2 = m_t1_v0 + m_t1_edge2; // Corner c2
    Vector3 v3 = m_t2_v0 + m_t2_edge1; // Corner c3 (calculated as c1 + edge_c3_c1)

    // Start bounds with the first vertex
    Vector3 min_p = v0;
    Vector3 max_p = v0;

    // Update bounds with the other distinct vertices
    updateBounds(v1, min_p, max_p);
    updateBounds(v2, min_p, max_p);
    updateBounds(v3, min_p, max_p);

    // Add epsilon for thickness
    const double epsilon = 1e-4;
    min_p.x -= epsilon; max_p.x += epsilon;
    min_p.y -= epsilon; max_p.y += epsilon;
    min_p.z -= epsilon; max_p.z += epsilon;

    output_box = AABB(min_p, max_p);
    return true;
}

// Epsilon for floating point comparisons
const double EPSILON = 1e-6;

Plane::Plane(const Vector3& c0, const Vector3& c1, const Vector3& c2, const Vector3& c3, const Material& mat) : m_material(mat)
{
    // Triangle 1 is (c0, c1, c2)
    m_t1_v0 = c0;
    m_t1_edge1 = c1 - c0;
    m_t1_edge2 = c2 - c0;

    // Triangle 2 is (c1, c3, c2)
    m_t2_v0 = c1;
    m_t2_edge1 = c3 - c1;
    m_t2_edge2 = c2 - c1;

    // Normal is calculated from the first triangle
    m_normal = m_t1_edge1.cross(m_t1_edge2).normalize();
}

// Möller-Trumbore ray-triangle intersection algorithm
bool Plane::rayTriangleIntersect(
    const Ray& ray, double t_min, double t_max,
    const Vector3& v0, const Vector3& edge1, const Vector3& edge2,
    double& out_t, double& out_u, double& out_v
) const {
    Vector3 h = ray.direction.cross(edge2);
    double a = edge1.dot(h);

    if (a > -EPSILON && a < EPSILON)
        return false; // Ray is parallel to this triangle.

    double f = 1.0 / a;
    Vector3 s = ray.origin - v0;
    double u = f * s.dot(h);

    if (u < 0.0 || u > 1.0)
        return false;

    Vector3 q = s.cross(edge1);
    double v = f * ray.direction.dot(q);

    if (v < 0.0 || u + v > 1.0)
        return false;

    // compute t to find out where the intersection point is.
    out_t = f * edge2.dot(q);

    if (out_t > t_min && out_t < t_max) {
        out_u = u;
        out_v = v;
        return true;
    }

    return false;
}


bool Plane::intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const {

    double t1, u1, v1;
    double t2, u2, v2;

    // Check intersection with the first triangle
    bool hit1 = rayTriangleIntersect(ray, t_min, t_max, m_t1_v0, m_t1_edge1, m_t1_edge2, t1, u1, v1);

    // Check intersection with the second triangle
    bool hit2 = rayTriangleIntersect(ray, t_min, t_max, m_t2_v0, m_t2_edge1, m_t2_edge2, t2, u2, v2);

    if (!hit1 && !hit2) {
        return false; // Missed both
    }

    double t_hit;
    bool hit_triangle_1;

    if (hit1 && hit2) {
        if (t1 < t2) {  // hit triangle 1 first
            t_hit = t1;
            hit_triangle_1 = true;
        } else {
            t_hit = t2;     // hit triangle 2 first
            hit_triangle_1 = false;
        }
    } else if (hit1) {  // hit just triangle 1
        t_hit = t1;
        hit_triangle_1 = true;
    } else {
        t_hit = t2;     // hit just triangle 2
        hit_triangle_1 = false;
    }

    // Valid hit
    rec.t = t_hit;
    rec.point = ray.point_at_parameter(t_hit);

    // Check if ray is hitting from front or back
    rec.set_face_normal(ray, m_normal);

    rec.mat = m_material;

    if (hit_triangle_1) {
        rec.uv.u = u1;
        rec.uv.v = v1;
    } else {
        rec.uv.u = 1.0 - v2;
        rec.uv.v = u2 + v2;
    }
    return true;
}