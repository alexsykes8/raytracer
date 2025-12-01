//
// Created by alex on 21/11/2025.
//

#ifndef B216602_COMPLEX_PLANE_H
#define B216602_COMPLEX_PLANE_H


#include "hittable.h"
#include "../utilities/matrix4x4.h"
#include "material.h"

class ComplexPlane : public Shape {
public:
    // constructor for the complex plane.
    ComplexPlane(const Matrix4x4& transform, const Matrix4x4& inv_transform, const Material& mat, const Vector3& velocity, double shutter_time);

    // overrides the intersect method to handle ray marching and displacement mapping.
    virtual bool intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const override;
    // overrides the getBoundingBox method to account for potential displacement.
    virtual bool getBoundingBox(AABB &output_box) const override;

private:
    // the object-to-world transformation matrix.
    Matrix4x4 m_transform;
    // the world-to-object inverse transformation matrix.
    Matrix4x4 m_inverse_transform;
    // the inverse transpose of the transformation matrix, used for transforming normals.
    Matrix4x4 m_inverse_transpose;
    // the material properties of the plane.
    Material m_material;
    // the velocity of the plane for motion blur.
    Vector3 m_velocity;
    // the maximum displacement value for the bump map.
    double m_max_displacement;
    double m_shutter_time;

    // calculates the uv coordinates and base normal for a given point on the plane.
    void get_uv_and_normal(const Vector3& p, double& u, double& v, Vector3& normal) const;
    // calculates the signed distance from a point to the base (undisplaced) plane.
    double signed_distance_plane(const Vector3& p) const;
    // helper method to calculate the bounding box after applying transformations.
    bool getTransformedBoundingBox(AABB& output_box, const Vector3& min_p, const Vector3& max_p) const;
};


#endif //B216602_COMPLEX_PLANE_H