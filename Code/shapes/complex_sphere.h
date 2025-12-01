#ifndef B216602_COMPLEX_SPHERE_H
#define B216602_COMPLEX_SPHERE_H

#include "sphere.h"

class ComplexSphere : public Sphere {
public:
    // constructor for the complex sphere, inheriting from the base sphere class.
    ComplexSphere(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat, const Vector3& velocity, double shutter_time);

    // overrides the intersect method to handle ray marching and displacement mapping.
    virtual bool intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const override;
    // overrides the getBoundingBox method to account for potential displacement.
    virtual bool getBoundingBox(AABB& output_box) const override;

private:
    // the maximum displacement value for the bump map.
    double m_max_displacement;

};

#endif //B216602_COMPLEX_SPHERE_H