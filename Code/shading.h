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
#include <cstdlib>

#ifndef B216602_SHADING_H
#define B216602_SHADING_H

inline double random_double () {
    return rand() / (RAND_MAX + 1.0);
}

inline Vector3 random_in_unit_sphere() {
    while (true) {
        auto p = Vector3(random_double() * 2 - 1, random_double() * 2 - 1, random_double() * 2 - 1);
        if (p.dot(p) < 1.0)
            return p;
    }
}

inline Vector3 random_point_on_light(const PointLight& light) {
    if (light.radius == 0.0) {
        return light.position;
    }
    return light.position + random_in_unit_sphere().normalize() * light.radius;
}

inline Vector3 component_wise_multiply(const Vector3& a, const Vector3& b) {
    return Vector3(a.x * b.x, a.y * b.y, a.z * b.z);
}

// calculates the local ambient diffuse. It computes the direct illumination component of the surface colour at the ray-hit point.
inline Vector3 calculate_local_ad(const HitRecord& rec, const Scene& scene, const HittableList& world) {

    const Material& mat = rec.mat;

    // first the base diffuse colour is found.
    Vector3 diffuse_colour;

    // if a texture is present this is prioritised.
    if (mat.texture) {
        // Get the (u,v) coordinates from the hit record for the texture
        double u = rec.uv.u;
        double v = rec.uv.v;

        // Get the texture dimensions
        int tex_width = mat.texture->getWidth();
        int tex_height = mat.texture->getHeight();

        // Convert (u,v) [0,1] to pixel (x,y) coordinates
        int x = static_cast<int>(u * (tex_width - 1));
        int y = static_cast<int>((1.0 - v) * (tex_height - 1));

        // Clamp values
        x = std::min(std::max(x, 0), tex_width - 1);
        y = std::min(std::max(y, 0), tex_height - 1);

        // Get the pixel from the texture
        Pixel tex_pixel = mat.texture->getPixel(x, y);

        // Convert the Pixel (0-255) back to a Vector3 (0-1)
        diffuse_colour = Vector3(tex_pixel.r / 255.0,
                                tex_pixel.g / 255.0,
                                tex_pixel.b / 255.0);
    } else {
        // No texture, just use the material's diffuse color
        diffuse_colour = mat.diffuse;
    }

    // The ambient component is an approximation of light scattered throughout the scene. Uses a low-level illumination to simulate light-scattering.
    // Ambient Component
    Vector3 global_ambient_light(0.2, 0.2, 0.2);
    Vector3 final_colour_vec = component_wise_multiply(mat.ambient, global_ambient_light);


    const Vector3 P = rec.point;
    const Vector3 N = rec.normal.normalize();
    double exposure = scene.getExposure();

    const int SHADOW_SAMPLES = 16;

    // rather than hard shadows, use a monte carlo sampling technique to implement soft shadows.
    for (const auto& light : scene.getLights()) {
        // default for if shadows are turned off
        double shadow_factor = 1.0;

        if (scene.shadows_enabled()) {
            shadow_factor = 0.0;
            for (int i = 0; i < SHADOW_SAMPLES; i++) {
                // randomly select a point on the surface of the light.
                Vector3 point_on_light = random_point_on_light(light);
                // calculates the vector from P (the hit point that was visible from the camera) to the light
                Vector3 shadow_ray_dir = point_on_light - P;
                // gets the distance to the light
                double dist_to_light = shadow_ray_dir.length();
                // normalizes the direction vector
                shadow_ray_dir = shadow_ray_dir.normalize();
                // creates a shadow ray from P
                Ray shadow_ray(P, shadow_ray_dir);
                HitRecord shadow_rec;
                // performs an intersection check against the whole world.
                if (!world.intersect(shadow_ray, 0.001, dist_to_light - 0.001, shadow_rec)) {
                    // if there is no object blocking the way to the light source, the shadow_factor is incremented (less shadow)
                    shadow_factor += 1.0;
                }
            }
            // the factor is divided by the number of samples. This creates the gradient between blocked and unblocked areas, as in these areas some rays from the light to the area may succeed but the other randomly selected light points may fail.
            shadow_factor /= SHADOW_SAMPLES;
        }
        // calculate the final, integrated caclulation of the diffuse light intensity.
        if (shadow_factor > 0) {
            // calculates the vector from P (the hit point that was visible from the camera) to the light
            Vector3 L_raw = light.position - P;
            // calculates the squared distance to the light
            double distance_squared = L_raw.dot(L_raw);
            // calculates the inverse square law of light falloff. Light intensity decreases proportional to the square of the distance from the source.
            double falloff = 1.0 / distance_squared;
            Vector3 L = L_raw.normalize(); // Normalized light vector
            // scales down the lights intensity using the falloff value and the exposure setting (passed as a command line argument) which controls the overall scene brightness
            Vector3 light_intensity_at_point = light.intensity * falloff * exposure;
            // Diffuse shading Lambert's cosine law
            double L_dot_N = std::max(0.0, L.dot(N)); // calculates the dot product of the light vector and the normal vector. this is the cosine of the angle between the light direction and the surface normal.
            // If the light hits perpendicularly, cos(theta) is 1.0 (max brightness). The closer the ray is to parallel with the surface, the closer it is to 0.0 (low brightness)

            // multiplies the diffuse colour by the light intensity of each colour. Scales by L_dot_N to account for surface angle, then applies shadow factor.
            Vector3 diffuse = component_wise_multiply(diffuse_colour, light_intensity_at_point) * L_dot_N * shadow_factor; // multiplied by shadow factor to reduce intensity based on visibility to the light source.

            // add to the final_colour_vec, which already contains ambient light and contributions from other lights.
            final_colour_vec = final_colour_vec + diffuse;
        }
    }

    return final_colour_vec;
}
inline Vector3 calculate_specular(const HitRecord& rec, const Scene& scene, const HittableList& world, const Ray& view_ray) {
    const Vector3 P = rec.point;
    const Vector3 N = rec.normal.normalize();
    const Vector3 V = (view_ray.origin - P).normalize();
    const Material& mat = rec.mat;
    double exposure = scene.getExposure();

    Vector3 specular_colour(0, 0, 0);
    const int SHADOW_SAMPLES = 16;

    for (const auto& light : scene.getLights()) {

        double shadow_factor = 1.0;

        if (scene.shadows_enabled()) {
            shadow_factor = 0.0;
            for (int i = 0; i < SHADOW_SAMPLES; ++i) {

                Vector3 point_on_light = random_point_on_light(light);
                Vector3 shadow_ray_dir = point_on_light - P;
                double dist_to_light = shadow_ray_dir.length();
                shadow_ray_dir = shadow_ray_dir.normalize();

                Ray shadow_ray(P, shadow_ray_dir);
                HitRecord shadow_rec;

                if (!world.intersect(shadow_ray, 0.001, dist_to_light - 0.001, shadow_rec)) {
                    shadow_factor += 1.0;
                }
            }
            shadow_factor /= SHADOW_SAMPLES;
        }

        if (shadow_factor > 0) {
            Vector3 L_raw = light.position - P;
            double distance_squared = L_raw.dot(L_raw);
            double falloff = 1.0 / distance_squared;
            Vector3 L = L_raw.normalize();
            Vector3 H = (L + V).normalize();

            Vector3 light_intensity_at_point = light.intensity * falloff * exposure;

            double H_dot_N = std::max(0.0, H.dot(N));
            Vector3 specular = component_wise_multiply(mat.specular, light_intensity_at_point) * std::pow(H_dot_N, mat.shininess) * shadow_factor;

            specular_colour = specular_colour + specular;
        }
    }

    return specular_colour;
}
#endif //B216602_SHADING_H