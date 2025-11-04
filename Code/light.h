//
// Created by alex on 04/11/2025.
//

#include "vector3.h"

#ifndef B216602_LIGHT_H
#define B216602_LIGHT_H


struct PointLight {
    Vector3 position;
    Vector3 intensity;

    PointLight() : position(0.0, 0.0, 0.0), intensity(1.0, 1.0, 1.0) {}

    PointLight(const Vector3& pos, const Vector3& intensity) : position(pos), intensity(intensity) {}
};

#endif //B216602_LIGHT_H