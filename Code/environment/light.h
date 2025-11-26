//
// Created by alex on 04/11/2025.
//

#include "../utilities/vector3.h"

#ifndef B216602_LIGHT_H
#define B216602_LIGHT_H


// 'struct' defines a composite data type that groups variables under a single name.
// represents a point light source in the scene, which can have a radius for soft shadows.
struct PointLight {
    // the 3d world-space position of the light source.
    Vector3 position;
    // the colour and brightness of the light.
    Vector3 intensity;
    // the radius of the light source. a non-zero radius creates an area light for soft shadows.
    double radius;

    // default constructor.
    // initialises a white light at the origin with zero radius.
    PointLight() : position(0.0, 0.0, 0.0), intensity(1.0, 1.0, 1.0), radius(0.0) {}

    // parameterised constructor.
    // initialises the light with a specified position, intensity, and radius.
    PointLight(const Vector3& pos, const Vector3& intensity, double rad) : position(pos), intensity(intensity), radius(rad) {}
};

#endif //B216602_LIGHT_H