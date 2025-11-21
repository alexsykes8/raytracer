//
// Created by alex on 21/11/2025.
//

#ifndef B216602_COMPLEX_PLANE_H
#define B216602_COMPLEX_PLANE_H


#include "hittable.h"
#include "../matrix4x4.h"
#include "../material.h"

class ComplexPlane : public Shape {
public:
    ComplexPlane(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat, const Vector3& velocity);

    virtual bool intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const override;
    virtual bool getBoundingBox(AABB &output_box) const override;

private:
    Matrix4x4 m_transform;
    Matrix4x4 m_inverse_transform;
    Matrix4x4 m_inverse_transpose;
    Material m_material;
    Vector3 m_velocity;
    double m_max_displacement;

    void get_uv_and_normal(const Vector3& p, double& u, double& v, Vector3& normal) const;
    double signed_distance_plane(const Vector3& p) const;
    bool getTransformedBoundingBox(AABB& output_box, const Vector3& min_p, const Vector3& max_p) const;
};


#endif //B216602_COMPLEX_PLANE_H