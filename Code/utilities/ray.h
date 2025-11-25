//
// Created by alex on 16/10/2025.
//

#ifndef B216602_RAY_H
#define B216602_RAY_H

#include "vector3.h"

/**
 * @brief Represents a ray in 3D space: R(t) = origin + t * direction.
 * The direction vector (D) is normalized for use in later calculations.
 */
struct Ray {
    // The starting point of the ray (the camera's location in world space).
    Vector3 origin;

    // The unit vector defining the direction of the ray.
    Vector3 direction;

    double time;

    // Constructor to create a Ray object
    Ray(const Vector3& o, const Vector3& d, double t = 0.0) : origin(o), direction(d), time(t) {}

    // Default constructor
    Ray() : origin(Vector3()), direction(Vector3()), time(0.0) {}

    //Returns a 3D point on the ray at distance t.
    Vector3 point_at_parameter(double t) const {
        return origin + (t * direction);
    }
};

#endif //B216602_RAY_H