//
// Created by alex on 29/10/2025.
//

#ifndef B216602_HITTABLE_H
#define B216602_HITTABLE_H

#include "../utilities/vector3.h"
#include "../utilities/ray.h"
#include "../acceleration/aabb.h"
#include "material.h"
#include "../utilities/vector2.h"

// 'struct' is a c++ keyword that defines a composite data type that groups variables under a single name.
// a structure to store information about a ray-object intersection.
struct HitRecord {
    // the distance along the ray from its origin to the intersection point.
    double t;
    // the 3d coordinate of the intersection point in world space.
    Vector3 point;
    // the surface normal vector at the intersection point. this normal is always oriented to face the incoming ray.
    Vector3 normal;
    // the material properties of the intersected object.
    Material mat;

    // the 2d texture coordinates (u, v) at the intersection point.
    Vector2 uv;

    // a boolean indicating if the ray hit the front face of the surface.
    bool front_face;

    // 'inline' is a c++ keyword that suggests to the compiler that a function's body should be inserted directly at the call site to reduce function call overhead.
    // 'const' specifies that a variable's value is constant and tells the compiler to prevent anything from modifying it.
    // '&' declares a reference variable. a reference is an alias for an already existing variable.
    inline void set_face_normal(const Ray& ray, const Vector3& outward_normal) {
        // determines if the ray hits the front or back of the surface.
        // if the dot product is negative, the ray and normal are pointing in opposite directions (front face hit).
        // equation: front_face = (ray.direction · outward_normal) < 0.0
        front_face = ray.direction.dot(outward_normal) < 0.0;
        // sets the normal to always point against the incoming ray.
        normal = front_face ? outward_normal : -outward_normal;
    }
};

// an abstract base class for any object in the scene that can be intersected by a ray.
class Shape {
public:
    // 'public' is a c++ keyword that makes members accessible from outside the class.
    // 'virtual' is a c++ keyword that indicates a member function can be overridden in a derived class.
    // a pure virtual function to test for an intersection between a ray and the shape.
    // the '= 0' makes this a pure virtual function, requiring derived classes to implement it.
    virtual bool intersect(
        const Ray& ray, // the ray to test against.
        double t_min, // the minimum valid distance for a hit.
        double t_max, // the maximum valid distance for a hit.
        HitRecord& rec // the hitrecord struct to be filled with data if a hit occurs.
        ) const = 0; // 'const' at the end of a member function declaration means the function will not modify the state of the object it is called on.
    // a pure virtual function to calculate the world-space axis-aligned bounding box (aabb) of the shape.
    virtual bool getBoundingBox(AABB& output_box) const = 0;
    // a virtual destructor to ensure proper cleanup when deleting a derived object through a base class pointer.
    // '~' denotes a destructor.
    virtual ~Shape() {} // an empty destructor body.
};

#endif //B216602_HITTABLE_H