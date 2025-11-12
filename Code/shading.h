//
// Created by alex on 04/11/2025.
//
#include "material.h"
#include "light.h"
#include "scene.h"
#include "ray.h"
#include "Image.h"
#include "shapes/hittable.h"
#include "shapes/hittable_list.h"
#include "vector3.h"

#include <cmath>
#include <algorithm>

#ifndef B216602_SHADING_H
#define B216602_SHADING_H


inline Vector3 component_wise_multiply(const Vector3& a, const Vector3& b) {
    return Vector3(a.x * b.x, a.y * b.y, a.z * b.z);
}

inline Vector3 blinn_phong_shade(const HitRecord& rec, const Scene& scene, const HittableList& world, const Ray& view_ray) {
    const Vector3 P = rec.point;
    const Vector3 N = rec.normal.normalize();
    const Vector3 V = (view_ray.origin - P).normalize();
    const Material& mat = rec.mat;
    const double exposure = scene.getExposure();

    // Ambient Component
    Vector3 global_ambient_light(0.2, 0.2, 0.2);
    Vector3 final_color_vec = component_wise_multiply(mat.ambient, global_ambient_light);

    // Loop over all lights for Diffuse and Specular
    for (const auto& light : scene.getLights()) {
        double shadow_factor = 1.0;
        if (scene.getEnableShadows()) { // Check if shadows are on
            Vector3 shadow_ray_dir = (light.position - P).normalize();
            double dist_to_light = (light.position - P).length();
            Ray shadow_ray(P, shadow_ray_dir);
            HitRecord shadow_rec;

            // Check for intersection, ignoring objects very close or past the light
            if (world.intersect(shadow_ray, 0.001, dist_to_light - 0.001, shadow_rec)) {
                shadow_factor = 0.0; // We are in shadow
            }
        }

        // Only add light if not in shadow
        if (shadow_factor > 0) {
            Vector3 L = (light.position - P).normalize();
            Vector3 H = (L + V).normalize();
            Vector3 exposed_light_intensity = light.intensity * exposure;

            // Diffuse
            double L_dot_N = std::max(0.0, L.dot(N));
            Vector3 diffuse = component_wise_multiply(mat.diffuse, exposed_light_intensity) * L_dot_N;

            // Specular
            double H_dot_N = std::max(0.0, H.dot(N));
            Vector3 specular = component_wise_multiply(mat.specular, exposed_light_intensity) * std::pow(H_dot_N, mat.shininess);

            final_color_vec = final_color_vec + (diffuse + specular) * shadow_factor;
        }
    }
    return final_color_vec;
}
#endif //B216602_SHADING_H