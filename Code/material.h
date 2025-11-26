//
// Created by alex on 04/11/2025.
//

#ifndef B216602_MATERIAL_H
#define B216602_MATERIAL_H

#include "vector3.h"

// defines the Blinn-Phong material properties
struct Material {
    Vector3 ambient;
    Vector3 diffuse;
    Vector3 specular;
    double shininess;

    double reflectivity = 0.0;
    double transparency = 0.0;
    double refractive_index = 1.0;

    Material() : ambient(0.1, 0.1, 0.1), diffuse(0.7, 0.7, 0.7), specular(1.0, 1.0, 1.0), shininess(32.0) {}
};

#endif //B216602_MATERIAL_H