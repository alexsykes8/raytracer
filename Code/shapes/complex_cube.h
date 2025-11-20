//
// Created by alex on 19/11/2025.
//

#ifndef B216602_COMPLEX_CUBE_H
#define B216602_COMPLEX_CUBE_H

#include "hittable.h"
#include "../vector3.h"
#include "../matrix4x4.h"
#include "../acceleration/aabb.h"
#include "../material.h"

class ComplexCube : public Shape {
public:
    ComplexCube(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat, const Vector3& velocity)
        : m_transform(transform),
          m_inverse_transform(inv_transform),
          m_inverse_transpose(inv_transform.transpose()),
          m_material(mat),
          m_velocity(velocity)
    {}

    virtual bool intersect(
        const Ray& ray,
        double t_min,
        double t_max,
        HitRecord& rec
    ) const override;

    virtual bool getBoundingBox(AABB &output_box) const override;
    virtual bool any_hit(const Ray& ray, double t_min, double t_max) const override;

private:
    Matrix4x4 m_transform;
    Matrix4x4 m_inverse_transform;
    Matrix4x4 m_inverse_transpose;
    Material m_material;
    Vector3 m_velocity;

    const double MAX_DISPLACEMENT = 0.2;

    void get_uv_and_normal(const Vector3& p, double& u, double& v, Vector3& normal) const;

    double signed_distance_box(const Vector3& p) const;
};

#endif //B216602_COMPLEX_CUBE_H