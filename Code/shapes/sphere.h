//
// Created by alex on 29/10/2025.
//

#ifndef B216602_SPHERE_H
#define B216602_SPHERE_H

#include "../acceleration/aabb.h"
#include "transformed_shape.h"


class Sphere : public TransformedShape {
public:
    Sphere(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat, const Vector3& velocity);

    virtual bool intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const override;
    virtual bool getBoundingBox(AABB &output_box) const override;

protected:
    static void get_sphere_uv(const Vector3& p, double& u, double& v);
};

#endif //B216602_SPHERE_H