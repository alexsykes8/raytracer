//
// Created by alex on 04/11/2025.
//

#ifndef B216602_MATERIAL_H
#define B216602_MATERIAL_H

#include "vector3.h"
#include <string>
#include <memory>


class Image;

// defines the Blinn-Phong material properties
struct Material {
    // Glass should not have ambient or diffuse
    Vector3 ambient;    // each component range is [0,1]. This is the colour when hit by global ambient light. It should be about 10% of diffuse.
    Vector3 diffuse;    // each component range is [0,1]. This is the colour when hit by light. (base colour)
    Vector3 specular;   // each component range is [0,1]. low specular is more matt
    double shininess;   // component range is [0,inf]
    // if specular is black [0,0,0] then you will not see any shininess.

    double reflectivity = 0.0;  // 0.0 is not reflective 1.0 is reflective
    double transparency = 0.0;  // 0.0 is opaque 1.0 is transparent
    // reflectivity and transparency cannot add to more than 1

    double refractive_index = 1.0;  // 1.0 is no refraction, 1.33 is water, 1.52 is glass, 2.42 is a diamond

    std::string texture_filename;
    std::shared_ptr<Image> texture;

    std::string bump_map_filename;
    std::shared_ptr<Image> bump_map;

    Material() : ambient(0.1, 0.1, 0.1), diffuse(0.7, 0.7, 0.7), specular(1.0, 1.0, 1.0), shininess(32.0), texture(nullptr), bump_map(nullptr) {}};

#endif //B216602_MATERIAL_H