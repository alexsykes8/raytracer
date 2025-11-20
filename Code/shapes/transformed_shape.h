#ifndef B216602_TRANSFORMED_SHAPE_H
#define B216602_TRANSFORMED_SHAPE_H

#include "hittable.h"
#include "../matrix4x4.h"
#include "../material.h"
#include "../vector3.h"
#include "../acceleration/aabb.h"
#include <limits>
class TransformedShape : public Shape {
public:
    TransformedShape(const Matrix4x4& transform, const Matrix4x4& inv_transform,
                     const Material& mat, const Vector3& velocity)
        : m_transform(transform),
          m_inverse_transform(inv_transform),
          m_inverse_transpose(inv_transform.transpose()),
          m_material(mat),
          m_velocity(velocity)
    {}

protected:
    Matrix4x4 m_transform;
    Matrix4x4 m_inverse_transform;
    Matrix4x4 m_inverse_transpose;
    Material m_material;
    Vector3 m_velocity;

    bool getTransformedBoundingBox(AABB& output_box, const Vector3& local_min, const Vector3& local_max) const {
        double infinity = std::numeric_limits<double>::infinity();
        Vector3 min_p(infinity, infinity, infinity);
        Vector3 max_p(-infinity, -infinity, -infinity);

        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                for (int k = 0; k < 2; ++k) {
                    Vector3 corner(
                        (i == 0) ? local_min.x : local_max.x,
                        (j == 0) ? local_min.y : local_max.y,
                        (k == 0) ? local_min.z : local_max.z
                    );
                    Vector3 transformed_corner = m_transform * corner;
                    AABB::updateBounds(transformed_corner, min_p, max_p);
                }
            }
        }

        output_box = AABB(min_p, max_p);
        return true;
    }
};

#endif //B216602_TRANSFORMED_SHAPE_H