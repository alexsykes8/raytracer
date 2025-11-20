//
// Created by alex on 19/11/2025.
//

#ifndef B216602_COMPLEX_CUBE_H
#define B216602_COMPLEX_CUBE_H

#include "cube.h"

class ComplexCube : public Cube {
public:
    ComplexCube(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat, const Vector3& velocity);

    virtual bool intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const override;
    virtual bool getBoundingBox(AABB &output_box) const override;

private:
    double m_max_displacement;
    void get_uv_and_normal(const Vector3& p, double& u, double& v, Vector3& normal) const;
    double signed_distance_box(const Vector3& p) const;
};


#endif //B216602_COMPLEX_CUBE_H