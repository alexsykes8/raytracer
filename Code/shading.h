//
// Created by alex on 04/11/2025.
//
#include "material.h"
#include "light.h"
#include "scene.h"
#include "ray.h"
#include "Image.h"
#include "shapes/hittable.h"
#include "vector3.h"

#include <cmath>
#include <algorithm>

#ifndef B216602_SHADING_H
#define B216602_SHADING_H

inline Vector3 component_wise_multiply(const Vector3& a, const Vector3& b) {
    return Vector3(a.x * b.x, a.y * b.y, a.z * b.z);
}

inline Pixel blinn_phong_shade(const HitRecord& rec, const Scene& scene, const Ray& view_ray) {
    const Vector3 P = rec.point;                          // Intersection point
    const Vector3 N = rec.normal.normalize();         // Surface normal
    const Vector3 V = (view_ray.origin - P).normalize(); // View vector

    const Material& mat = rec.mat;

    const double exposure = scene.getExposure();

    // Ambient Component
    Vector3 global_ambient_light(0.2, 0.2, 0.2);
    Vector3 final_color_vec = component_wise_multiply(mat.ambient, global_ambient_light);

    // Loop over all lights for Diffuse and Specular
    for (const auto& light : scene.getLights()) {
        Vector3 exposed_light_intensity = light.intensity * exposure;

        Vector3 L = (light.position - P).normalize(); // Light vector
        Vector3 H = (L + V).normalize();              // Halfway vector

        // Diffuse Component
        double L_dot_N = std::max(0.0, L.dot(N));
        Vector3 diffuse = component_wise_multiply(mat.diffuse, exposed_light_intensity) * L_dot_N;

        // Specular Component
        double H_dot_N = std::max(0.0, H.dot(N));
        Vector3 specular = component_wise_multiply(mat.specular, exposed_light_intensity) * std::pow(H_dot_N, mat.shininess);

        // Add this light's contribution
        final_color_vec = final_color_vec + diffuse + specular;
    }

    // Convert final vector to Pixel
    // Clamp color values to [0, 1] before scaling to [0, 255]
    auto clamp = [](double val) { return std::max(0.0, std::min(1.0, val)); };

    unsigned char r = static_cast<unsigned char>(clamp(final_color_vec.x) * 255.0);
    unsigned char g = static_cast<unsigned char>(clamp(final_color_vec.y) * 255.0);
    unsigned char b = static_cast<unsigned char>(clamp(final_color_vec.z) * 255.0);

    return {r, g, b};
}
#endif //B216602_SHADING_H