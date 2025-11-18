//
// Created by alex on 29/10/2025.
//

#ifndef B216602_HITTABLE_H
#define B216602_HITTABLE_H

#include "../vector3.h"
#include "../ray.h"
#include "../acceleration/aabb.h"
#include "../material.h"
#include "../vector2.h"

struct HitRecord {
    double t;       // Distance from ray origin to intersection point
    double u;       // Texture coordinate U
    double v;       // Texture coordinate V
    Vector3 point;  // 3D coord of the intersection point
    Vector3 normal; // Normal vector at the intersection point
    Material mat;
    bool front_face;

    inline void set_face_normal(const Ray& ray, const Vector3& outward_normal) {
        // Sets the hit record normal to always point against the ray
        front_face = ray.direction.dot(outward_normal) < 0.0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class Shape {
public:
    // Tests for an intersection between a ray and the shape
    virtual bool intersect(
        const Ray& ray, // The ray to test against
        double t_min, // the min valid distance for a hit
        double t_max, // the max valid distance for a hit
        HitRecord& rec // the HitRecord struct to be filled with data if a hit occurs
        ) const = 0;
    virtual bool getBoundingBox(AABB& output_box) const = 0;
    virtual ~Shape() {}
};

#endif //B216602_HITTABLE_H