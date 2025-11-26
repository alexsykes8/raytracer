//
// Created by alex on 29/10/2025.
//

#ifndef B216602_SPHERE_H
#define B216602_SPHERE_H

#include "../acceleration/aabb.h"
#include "transformed_shape.h"


// represents a sphere shape that can be transformed (scaled, rotated, translated).
class Sphere : public TransformedShape {
public:
    // constructor for the sphere.
    Sphere(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat, const Vector3& velocity);

    virtual bool intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const override;
    virtual bool getBoundingBox(AABB &output_box) const override;

protected:
    // calculates the spherical texture coordinates (u, v) for a given point on the sphere's surface.
    static void get_sphere_uv(const Vector3& p, double& u, double& v);
};

#endif //B216602_SPHERE_H