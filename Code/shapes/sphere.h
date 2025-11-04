//
// Created by alex on 29/10/2025.
//

#ifndef B216602_SPHERE_H
#define B216602_SPHERE_H

#include "hittable.h"
#include "../matrix4x4.h"
#include "../acceleration/aabb.h"
#include "../material.h"

class Sphere : public Shape {
public:
    // initialises the object-to-world transform and inverse
    Sphere(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat)
        : m_transform(transform),
          m_inverse_transform(inv_transform),
          m_inverse_transpose(inv_transform.transpose()),//precalculates inverse transpose for later
          m_material(mat)
    {}

    // virtual enables polymorphism

    // returns true for a hit, false for a miss
    virtual bool intersect(
        const Ray& ray,
        double t_min,
        double t_max,
        // details about the intersection point
        HitRecord& rec
    ) const override;

    virtual bool getBoundingBox(AABB &output_box) const override;

private:
    Matrix4x4 m_transform;         // M: Object-to-World
    Matrix4x4 m_inverse_transform; // M_inverse: World-to-Object
    Matrix4x4 m_inverse_transpose; // (M_inverse)^T
    Material m_material;
};

#endif //B216602_SPHERE_H