#ifndef B216602_MATRIX4X4_H
#define B216602_MATRIX4X4_H

#include "vector3.h"
#include <cmath>
#include <stdexcept>


class Matrix4x4 {
public:
    double m[4][4];

    // Default constructor (initializes to Identity)
    Matrix4x4();

    // Declares static methods belonging to the Matrix4x4 class.
    static Matrix4x4 createTranslation(const Vector3& t);
    static Matrix4x4 createScale(const Vector3& s);
    static Matrix4x4 createRotationX(double radians);
    static Matrix4x4 createRotationY(double radians);
    static Matrix4x4 createRotationZ(double radians);


    // Matrix-Matrix Multiplication
    Matrix4x4 operator*(const Matrix4x4& other) const;

    // Matrix-Vector Multiplication to transform a point.
    Vector3 operator*(const Vector3& v) const;

    //Transforms a direction vector (w=0).
    Vector3 transformDirection(const Vector3& v) const;

    // Calculates the transpose of the matrix.
    Matrix4x4 transpose() const;

    //Calculates the inverse of the matrix.
    //Throws a runtime_error if the matrix is singular (not invertible).
    Matrix4x4 inverse() const;
};

#endif //B216602_MATRIX4X4_H