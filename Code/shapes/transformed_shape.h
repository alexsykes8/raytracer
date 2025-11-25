#ifndef B216602_TRANSFORMED_SHAPE_H
#define B216602_TRANSFORMED_SHAPE_H

#include "hittable.h"
#include "../utilities/matrix4x4.h"
#include "material.h"
#include "../utilities/vector3.h"
#include "../acceleration/aabb.h"
#include <limits>

// an abstract base class for shapes that can be translated, rotated, and scaled.
// it handles the transformation logic, while derived classes implement the specific geometry.
class TransformedShape : public Shape {
public:
    // 'public' makes members accessible from outside the class.
    // constructor for a transformed shape.
    // 'const' specifies that a variable's value is constant and tells the compiler to prevent anything from modifying it.
    // '&' declares a reference variable. a reference is an alias for an already existing variable.
    TransformedShape(const Matrix4x4& transform, const Matrix4x4& inv_transform,
                     const Material& mat, const Vector3& velocity)
        // initialises member variables with the provided parameters.
        : m_transform(transform),
          m_inverse_transform(inv_transform),
          // pre-calculates the inverse transpose matrix, used for transforming normals correctly.
          m_inverse_transpose(inv_transform.transpose()),
          m_material(mat),
          m_velocity(velocity)
    {}

protected:
    // 'protected' makes members accessible only within this class and its derived classes.
    // the object-to-world transformation matrix.
    Matrix4x4 m_transform;
    // the world-to-object inverse transformation matrix.
    Matrix4x4 m_inverse_transform;
    // the inverse transpose of the transformation matrix.
    Matrix4x4 m_inverse_transpose;
    // the material properties of the shape.
    Material m_material;
    // the velocity of the shape for motion blur.
    Vector3 m_velocity;

    // calculates the world-space axis-aligned bounding box (aabb) for a given local-space box.
    bool getTransformedBoundingBox(AABB& output_box, const Vector3& local_min, const Vector3& local_max) const {
        // initialises the world-space bounding box extents to infinity.
        double infinity = std::numeric_limits<double>::infinity();
        Vector3 min_p(infinity, infinity, infinity);
        Vector3 max_p(-infinity, -infinity, -infinity);

        // iterates through the 8 corners of the local-space bounding box.
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                for (int k = 0; k < 2; ++k) {
                    // constructs a corner by selecting from the min/max local bounds.
                    Vector3 corner(
                        (i == 0) ? local_min.x : local_max.x,
                        (j == 0) ? local_min.y : local_max.y,
                        (k == 0) ? local_min.z : local_max.z
                    );
                    // transforms the corner from local space to world space.
                    Vector3 transformed_corner = m_transform * corner;
                    // expands the world-space bounding box to include the transformed corner.
                    AABB::updateBounds(transformed_corner, min_p, max_p);
                }
            }
        }

        // creates the final aabb from the calculated min and max world-space points.
        output_box = AABB(min_p, max_p);
        return true;
    }
};

#endif //B216602_TRANSFORMED_SHAPE_H