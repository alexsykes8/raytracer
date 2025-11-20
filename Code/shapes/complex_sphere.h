#ifndef B216602_COMPLEX_SPHERE_H
#define B216602_COMPLEX_SPHERE_H

#include "hittable.h"
#include "../vector3.h"
#include "../matrix4x4.h"
#include "../acceleration/aabb.h"
#include "../material.h"

class ComplexSphere : public Shape {
public:
    ComplexSphere(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat, const Vector3& velocity)
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

    virtual bool getBoundingBox(AABB& output_box) const override;

private:
    Matrix4x4 m_transform;
    Matrix4x4 m_inverse_transform;
    Matrix4x4 m_inverse_transpose;
    Material m_material;
    Vector3 m_velocity;

    const double MAX_DISPLACEMENT = 0.15;

    void get_sphere_uv(const Vector3& p, double& u, double& v) const;
};

#endif //B216602_COMPLEX_SPHERE_H