#ifndef B216602_COMPLEX_SPHERE_H
#define B216602_COMPLEX_SPHERE_H

#include "sphere.h"

class ComplexSphere : public Sphere {
public:
    ComplexSphere(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat, const Vector3& velocity);

    virtual bool intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const override;
    virtual bool getBoundingBox(AABB& output_box) const override;

private:
    double m_max_displacement;

};

#endif //B216602_COMPLEX_SPHERE_H