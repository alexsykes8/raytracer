#ifndef B216602_CUBE_H
#define B216602_CUBE_H

#include "transformed_shape.h"
#include "../acceleration/aabb.h"


class Cube : public TransformedShape {
public:
    Cube(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat, const Vector3& velocity);

    virtual bool intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const override;
    virtual bool getBoundingBox(AABB &output_box) const override;
    virtual bool any_hit(const Ray& ray, double t_min, double t_max) const override;

};

#endif //B216602_CUBE_H