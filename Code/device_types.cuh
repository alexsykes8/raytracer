#pragma once

#include <iostream>
#include <iomanip>
#include <cmath>

class Vector3 {
public:
    double x, y, z;

    __host__ __device__ Vector3() : x(0.0), y(0.0), z(0.0) {}
    __host__ __device__ Vector3(double x_in, double y_in, double z_in) : x(x_in), y(y_in), z(z_in) {}
    __host__ __device__ Vector3(const Vector3& other) : x(other.x), y(other.y), z(other.z) {}
    __host__ __device__ Vector3& operator=(const Vector3& other) {
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }
    __host__ __device__ Vector3 operator-() const { return Vector3(-x, -y, -z); }
    __host__ __device__ Vector3 operator-(const Vector3& other) const { return Vector3(x - other.x, y - other.y, z - other.z); }
    __host__ __device__ Vector3 operator*(double t) const { return Vector3(x * t, y * t, z * t); }
    __host__ __device__ Vector3 operator+(const Vector3& other) const { return Vector3(x + other.x, y + other.y, z + other.z); }

    __host__ __device__ double dot(const Vector3& other) const { return x * other.x + y * other.y + z * other.z; }
    __host__ __device__ Vector3 cross(const Vector3& other) const {
        return Vector3(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
    }
    __host__ __device__ double length() const { return std::sqrt(x * x + y * y + z * z); }
    __host__ __device__ Vector3 normalize() const {
        double len = length();
        return (len > 1e-6) ? Vector3(x / len, y / len, z / len) : Vector3();
    }
};

inline __host__ __device__ Vector3 operator*(double t, const Vector3& v) {
    return v * t;
}

struct Ray {
    Vector3 origin;
    Vector3 direction;
    __host__ __device__ Ray(const Vector3& o, const Vector3& d) : origin(o), direction(d) {}
    __host__ __device__ Ray() : origin(Vector3()), direction(Vector3()) {}
    __host__ __device__ Vector3 point_at_parameter(double t) const { return origin + (t * direction); }
};

struct Material {
    Vector3 ambient;
    Vector3 diffuse;
    Vector3 specular;
    double shininess;
    double reflectivity;
    double transparency;
    double refractive_index;
    __host__ __device__ Material()
        : ambient(0.1, 0.1, 0.1), diffuse(0.7, 0.7, 0.7), specular(1.0, 1.0, 1.0),
          shininess(32.0), reflectivity(0.0), transparency(0.0), refractive_index(1.0) {}
};

struct PointLight {
    Vector3 position;
    Vector3 intensity;
};

struct HitRecord {
    double t;
    Vector3 point;
    Vector3 normal;
    Material mat;
};


class Matrix4x4 {
public:
    double m[4][4]; // row-major

    __host__ __device__ Matrix4x4() {
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                m[i][j] = (i == j) ? 1.0 : 0.0;
    }

    __host__ __device__ Matrix4x4 operator*(const Matrix4x4& other) const {
        Matrix4x4 result;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result.m[i][j] = 0.0;
                for (int k = 0; k < 4; ++k) {
                    result.m[i][j] += m[i][k] * other.m[k][j];
                }
            }
        }
        return result;
    }

    __host__ static Matrix4x4 createTranslation(const Vector3& t) {
        Matrix4x4 mat;
        mat.m[0][3] = t.x;
        mat.m[1][3] = t.y;
        mat.m[2][3] = t.z;
        return mat;
    }

    __host__ static Matrix4x4 createScale(const Vector3& s) {
        Matrix4x4 mat;
        mat.m[0][0] = s.x;
        mat.m[1][1] = s.y;
        mat.m[2][2] = s.z;
        return mat;
    }

    __host__ static Matrix4x4 createRotationX(double angle_rad) {
        Matrix4x4 mat;
        double c = cos(angle_rad);
        double s = sin(angle_rad);
        mat.m[1][1] = c;
        mat.m[1][2] = -s;
        mat.m[2][1] = s;
        mat.m[2][2] = c;
        return mat;
    }

    __host__ static Matrix4x4 createRotationY(double angle_rad) {
        Matrix4x4 mat;
        double c = cos(angle_rad);
        double s = sin(angle_rad);
        mat.m[0][0] = c;
        mat.m[0][2] = s;
        mat.m[2][0] = -s;
        mat.m[2][2] = c;
        return mat;
    }

    __host__ static Matrix4x4 createRotationZ(double angle_rad) {
        Matrix4x4 mat;
        double c = cos(angle_rad);
        double s = sin(angle_rad);
        mat.m[0][0] = c;
        mat.m[0][1] = -s;
        mat.m[1][0] = s;
        mat.m[1][1] = c;
        return mat;
    }

    __host__ Matrix4x4 inverse() const;
    __host__ Matrix4x4 transpose() const;

    __host__ void print() const {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                std::cout << std::setw(12) << std::fixed << std::setprecision(5) << m[i][j] << " ";
            }
            std::cout << std::endl;
        }
    }

    __host__ __device__ Vector3 transformPoint(const Vector3& p) const {
        double x = m[0][0] * p.x + m[0][1] * p.y + m[0][2] * p.z + m[0][3];
        double y = m[1][0] * p.x + m[1][1] * p.y + m[1][2] * p.z + m[1][3];
        double z = m[2][0] * p.x + m[2][1] * p.y + m[2][2] * p.z + m[2][3];
        double w = m[3][0] * p.x + m[3][1] * p.y + m[3][2] * p.z + m[3][3];
        return Vector3(x / w, y / w, z / w);
    }


    __host__ __device__ Vector3 transformVector(const Vector3& v) const {
        double x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z;
        double y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z;
        double z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z;
        return Vector3(x, y, z);
    }
};


struct Sphere {
    Matrix4x4 transform;
    Matrix4x4 inverse_transform;
    Matrix4x4 inverse_transpose;
    Material mat;
};

struct Plane {
    Vector3 p0, p1, p2, p3;
    Vector3 normal;
    Material mat;
};

struct Cube {
    Matrix4x4 transform;
    Matrix4x4 inverse_transform;
    Matrix4x4 inverse_transpose;
    Material mat;
};


struct CUDACamera {
    Vector3 origin;
    Vector3 lower_left_corner;
    Vector3 horizontal;
    Vector3 vertical;

    __device__ Ray get_ray(float u, float v) const {
        return Ray(origin, (lower_left_corner + u * horizontal + v * vertical - origin).normalize());
    }
};