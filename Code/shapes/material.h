//
// Created by alex on 04/11/2025.
//

#ifndef B216602_MATERIAL_H
#define B216602_MATERIAL_H

#include "../utilities/vector3.h"
#include <string>
#include <memory>


class Image; // forward declaration of the image class to avoid circular dependencies.

// 'struct' defines a composite data type that groups variables under a single name.
// defines the blinn-phong material properties.
struct Material {
    // the colour of the material under ambient lighting. range [0,1].
    Vector3 ambient;
    // the base colour of the material under direct diffuse lighting. range [0,1].
    Vector3 diffuse;
    // the colour of the specular highlight. low specular is more matt. range [0,1].
    Vector3 specular;
    // controls the size of the specular highlight. higher values create smaller, sharper highlights. range [0,inf].
    double shininess;

    // the amount of light reflected. 0.0 is not reflective, 1.0 is a perfect mirror.
    double reflectivity = 0.0;
    // the amount of light transmitted through the material. 0.0 is opaque, 1.0 is fully transparent.
    double transparency = 0.0;

    // the index of refraction (ior) of the material, used for snell's law. 1.0 is no refraction.
    double refractive_index = 1.0;

    // the filename of the texture map.
    std::string texture_filename;
    // 'std::shared_ptr' is a smart pointer that retains shared ownership of an object through a pointer.
    // a pointer to the loaded texture image.
    std::shared_ptr<Image> texture;

    // the filename of the bump map.
    std::string bump_map_filename;
    // a pointer to the loaded bump map image.
    std::shared_ptr<Image> bump_map;

    // a string identifier for the material type, e.g., "glass".
    std::string type = "glass";

    // default constructor.
    // initialises the material with default diffuse-like properties.
    Material() : ambient(0.1, 0.1, 0.1), diffuse(0.7, 0.7, 0.7), specular(1.0, 1.0, 1.0), shininess(32.0), texture(nullptr), bump_map(nullptr) {}};

#endif //B216602_MATERIAL_H