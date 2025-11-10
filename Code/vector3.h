//
// Created by alex on 16/10/2025.
//

#ifndef B216602_VECTOR3_H
#define B216602_VECTOR3_H



#include <cmath>
#include <iostream>

/**
 * @brief Represents a 3D vector or point (X, Y, Z). Also contains some operations for use in vector calculations.
 */
class Vector3 {
public:
    double x, y, z;

    // Default constructor (initializes to zero)
    Vector3() : x(0.0), y(0.0), z(0.0) {}

    // Parameterized constructor
    Vector3(double x_in, double y_in, double z_in) : x(x_in), y(y_in), z(z_in) {}

    // --- Vector Operations, for use later ---

    // Unary subtraction
    Vector3 operator-() const {
        return Vector3(-x, -y, -z);
    }

    // Vector subtraction
    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    // Scalar multiplication
    Vector3 operator*(double t) const {
        return Vector3(x * t, y * t, z * t);
    }

    // Scalar division
    Vector3 operator/(double t) const {
        return Vector3(x / t, y / t, z / t);
    }

    // Vector addition
    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    // Dot product
    double dot(const Vector3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    // Cross product
    Vector3 cross(const Vector3& other) const {
        return Vector3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    // Magnitude (length) of the vector
    double length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    // normalized (unit length) version of the vector
    Vector3 normalize() const {
        double len = length();
        if (len > 1e-6) { // Avoid division by zero
            return Vector3(x / len, y / len, z / len);
        }
        return Vector3(); // Return zero vector if length is zero
    }
};


// Global operator for scalar * vector
inline Vector3 operator*(double t, const Vector3& v) {
    return v * t;
}


#endif //B216602_VECTOR3_H