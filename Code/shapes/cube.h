#ifndef B216602_CUBE_H
#define B216602_CUBE_H

#include "transformed_shape.h"
#include "../acceleration/aabb.h"


class Cube : public TransformedShape {
public:
    // constructor for the cube.
    Cube(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat, const Vector3& velocity); // 'public' is a c++ keyword that makes members accessible from outside the class.

    // overrides the intersect method from the base 'shape' class to provide cube-specific intersection logic.
    virtual bool intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const override;
    // overrides the getboundingbox method to calculate the cube's world-space bounding box.
    virtual bool getBoundingBox(AABB &output_box) const override;

};

#endif //B216602_CUBE_H