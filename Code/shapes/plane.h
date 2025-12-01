#ifndef B216602_PLANE_H
#define B216602_PLANE_H

#include "hittable.h"
#include "../utilities/vector3.h"
#include <cmath> // For std::min
#include "../acceleration/aabb.h"
#include "material.h"

// represents a quadrilateral plane in 3d space, composed of two triangles.
class Plane : public Shape {
public:
    // constructor for the plane, defined by four corner vertices.
    // 'const' specifies that a variable's value is constant and tells the compiler to prevent anything from modifying it.
    // '&' declares a reference variable. a reference is an alias for an already existing variable.
    Plane(const Vector3& c0, const Vector3& c1, const Vector3& c2, const Vector3& c3, const Material& mat, const Vector3& velocity, double shutter_time);

    // 'virtual' indicates a member function can be overridden in a derived class.
    // overrides the base class method to test for ray-plane intersection.
    virtual bool intersect(
        const Ray& ray,
        double t_min,
        double t_max,
        HitRecord& rec
    ) const override; // 'const' at the end of a member function declaration means the function will not modify the state of the object it is called on.

    // overrides the base class method to calculate the plane's world-space bounding box.
    virtual bool getBoundingBox(AABB &output_box) const override;


private:
    // 'private' makes members accessible only within this class.
    // the four corner vertices of the plane.
    Vector3 m_c0, m_c1, m_c2, m_c3; // these are stored for potential future use but not directly used in intersection.

    // the geometric normal vector for the entire plane.
    Vector3 m_normal;

    // the plane is split into two triangles for intersection testing.

    // pre-calculated data for the first triangle (c0, c1, c2).
    Vector3 m_t1_v0, m_t1_edge1, m_t1_edge2;

    // pre-calculated data for the second triangle (c1, c3, c2).
    Vector3 m_t2_v0, m_t2_edge1, m_t2_edge2;

    // the material properties of the plane.
    Material m_material;

    // the velocity of the plane for motion blur.
    Vector3 m_velocity;

    double m_shutter_time;

    // a private helper function implementing the möller-trumbore ray-triangle intersection algorithm.
    bool rayTriangleIntersect(
        const Ray& ray, double t_min, double t_max,
        const Vector3& v0, const Vector3& edge1, const Vector3& edge2,
        double& out_t, double& out_u, double& out_v
    ) const;
};
#endif //B216602_PLANE_H