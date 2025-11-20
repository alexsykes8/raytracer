#ifndef B216602_CUBE_H
#define B216602_CUBE_H

#include "../shapes/hittable.h"
#include "../vector3.h"
#include "../matrix4x4.h"
#include "../acceleration/aabb.h"
#include "../material.h"

class Cube : public Shape {
public:
    // Constructor is identical to Sphere's
    Cube(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat,  const Vector3& velocity)
        : m_transform(transform),
          m_inverse_transform(inv_transform), // necessary to transform the ray into object space
          m_inverse_transpose(inv_transform.transpose()), // Initialize the inverse-transpose
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
};

#endif //B216602_CUBE_H