//
// Created by alex on 19/11/2025.
//

#ifndef B216602_COMPLEX_CUBE_H
#define B216602_COMPLEX_CUBE_H

#include "cube.h"

class ComplexCube : public Cube {
public:
    // constructor for the complex cube, inheriting from the base cube class.
    ComplexCube(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat, const Vector3& velocity, double shutter_time);

    // overrides the intersect method to handle ray marching and displacement mapping.
    virtual bool intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const override;
    // overrides the getBoundingBox method to account for potential displacement.
    virtual bool getBoundingBox(AABB &output_box) const override;

private:
    // the maximum displacement value for the bump map.
    double m_max_displacement;
    // calculates the uv coordinates and base normal for a given point on the cube.
    void get_uv_and_normal(const Vector3& p, double& u, double& v, Vector3& normal) const;
    // calculates the signed distance from a point to the base (undisplaced) cube.
    double signed_distance_box(const Vector3& p) const;
};


#endif //B216602_COMPLEX_CUBE_H