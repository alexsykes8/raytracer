//
// Created by alex on 16/10/2025.
//

#ifndef B216602_VECTOR3_H
#define B216602_VECTOR3_H



#include <cmath>
#include <iostream>


// represents a 3d vector for points, directions, and colours.
class Vector3 {
public:
    // the x, y, and z components of the 3d vector.
    double x, y, z;

    // default constructor.
    // initialises the vector to (0.0, 0.0, 0.0).
    Vector3() : x(0.0), y(0.0), z(0.0) {}

    // parameterised constructor.
    // initialises the vector with the given x, y, and z values.
    Vector3(double x_in, double y_in, double z_in) : x(x_in), y(y_in), z(z_in) {}

    // unary negation operator.
    // flips the direction of the vector.
    // equation: -v = (-x, -y, -z)
    // const dictates that this method will not change the state of the vector3 object it is called on.
    Vector3 operator-() const {
        // returns a new vector with each component negated.
        return Vector3(-x, -y, -z);
    }

    // vector subtraction operator.
    // equation: a - b = (a.x - b.x, a.y - b.y, a.z - b.z)
    Vector3 operator-(const Vector3& other) const {
        // returns a new vector representing the difference between this vector and another.
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    // scalar multiplication operator.
    // equation: v * t = (x * t, y * t, z * t)
    Vector3 operator*(double t) const {
        // returns a new vector scaled by the scalar value t.
        return Vector3(x * t, y * t, z * t);
    }

    // scalar division operator.
    // equation: v / t = (x / t, y / t, z / t)
    Vector3 operator/(double t) const {
        // returns a new vector scaled down by the scalar value t.
        return Vector3(x / t, y / t, z / t);
    }

    // vector addition operator.
    // equation: a + b = (a.x + b.x, a.y + b.y, a.z + b.z)
    Vector3 operator+(const Vector3& other) const {
        // returns a new vector representing the sum of this vector and another.
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    // calculates the dot product of this vector and another.
    // equation: a · b = a.x * b.x + a.y * b.y + a.z * b.z
    double dot(const Vector3& other) const {
        // returns a scalar value.
        return x * other.x + y * other.y + z * other.z;
    }

    // calculates the cross product of this vector and another.
    // equation: a x b = (a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x)
    Vector3 cross(const Vector3& other) const {
        // returns a new vector that is perpendicular to both original vectors.
        return Vector3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    // calculates the magnitude (length) of the vector.
    // equation: |v| = sqrt(x*x + y*y + z*z)
    double length() const {
        // returns the scalar length of the vector.
        return std::sqrt(x * x + y * y + z * z);
    }

    // calculates the normalised (unit length) version of the vector.
    // equation: v_norm = v / |v|
    Vector3 normalize() const {
        // calculates the length of the vector.
        double len = length();
        // checks if the length is greater than a small threshold to avoid division by zero.
        if (len > 1e-6) {
            // returns a new vector with each component divided by the length.
            return Vector3(x / len, y / len, z / len);
        }
        // returns a zero vector if the original vector's length is close to zero.
        return Vector3();
    }
};


// global operator for scalar * vector multiplication (t * v).
// allows for writing multiplication in the more conventional order.
inline Vector3 operator*(double t, const Vector3& v) {
    return v * t;
}


#endif //B216602_VECTOR3_H