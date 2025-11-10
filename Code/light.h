//
// Created by alex on 04/11/2025.
//

#include "vector3.h"

#ifndef B216602_LIGHT_H
#define B216602_LIGHT_H


struct PointLight {
    Vector3 position;
    Vector3 intensity;
    double radius;

    PointLight() : position(0.0, 0.0, 0.0), intensity(1.0, 1.0, 1.0), radius(0.0) {}

    PointLight(const Vector3& pos, const Vector3& intensity, double rad) : position(pos), intensity(intensity), radius(rad) {}
};

#endif //B216602_LIGHT_H