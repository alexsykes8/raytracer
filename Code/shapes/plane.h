#ifndef B216602_PLANE_H
#define B216602_PLANE_H

#include "hittable.h"
#include "../vector3.h"
#include <cmath> // For std::min
#include "../acceleration/aabb.h"
#include "../material.h"

class Plane : public Shape {
public:
    Plane(const Vector3& c0, const Vector3& c1, const Vector3& c2, const Vector3& c3, const Material& mat, const Vector3& velocity);

    virtual bool intersect(
        const Ray& ray,
        double t_min,
        double t_max,
        HitRecord& rec
    ) const override;

    virtual bool getBoundingBox(AABB &output_box) const override;


private:
    Vector3 m_c0, m_c1, m_c2, m_c3;

    Vector3 m_normal; // Normal for the whole plane

    // to account for the plane being a quad instead of guaranteed square/rectangle/parallelogram, split into triangles

    // Triangle 1 (c0, c1, c2)
    Vector3 m_t1_v0, m_t1_edge1, m_t1_edge2;

    // Triangle 2 (c1, c3, c2)
    Vector3 m_t2_v0, m_t2_edge1, m_t2_edge2;

    Material m_material;

    Vector3 m_velocity;

    bool rayTriangleIntersect(
        const Ray& ray, double t_min, double t_max,
        const Vector3& v0, const Vector3& edge1, const Vector3& edge2,
        double& out_t, double& out_u, double& out_v
    ) const;
};
#endif //B216602_PLANE_H