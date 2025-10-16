//
// Created by alex on 16/10/2025.
//

#ifndef B216602_RAY_H
#define B216602_RAY_H

#include "vector3.h" // Includes your custom Vector3 class

/**
 * @brief Represents a ray in 3D space: R(t) = origin + t * direction.
 * The direction vector (D) MUST be normalized (unit length).
 */
struct Ray {
    // O: The starting point of the ray (the camera's location in world space).
    Vector3 origin;

    // D: The unit vector defining the direction of the ray.
    Vector3 direction;

    // Constructor to easily create a Ray object
    Ray(const Vector3& o, const Vector3& d) : origin(o), direction(d) {}

    // Default constructor
    Ray() : origin(Vector3()), direction(Vector3()) {}

    /**
     * Calculates a point on the ray at distance t.
     * t is the distance parameter (t > 0 means the point is in front of the origin).
     * returns the 3D point R(t).
     */
    Vector3 point_at_parameter(double t) const {
        return origin + (t * direction);
    }
};

#endif //B216602_RAY_H