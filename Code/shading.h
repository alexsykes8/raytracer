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
#include "config.h"

#include <cmath>
#include <algorithm>

#include "random_utils.h"

#ifndef B216602_SHADING_H
#define B216602_SHADING_H




inline Vector3 random_point_on_light(const PointLight& light) {
    if (light.radius == 0.0) {
        return light.position;
    }
    return light.position + random_in_unit_sphere().normalize() * light.radius;
}

inline Vector3 component_wise_multiply(const Vector3& a, const Vector3& b) {
    return Vector3(a.x * b.x, a.y * b.y, a.z * b.z);
}

inline double trace_shadow_transmission(const Ray& shadow_ray, double dist_to_light, const HittableList& world) {
    double transmission = 1.0;
    Ray current_ray = shadow_ray;
    double current_dist = dist_to_light;

    while (true) {
        HitRecord rec;
        if (world.intersect(current_ray, 0.001, current_dist - 0.001, rec)) {

            if (rec.mat.transparency > 0.0) {
                transmission *= rec.mat.transparency;

                if (transmission < 0.001) return 0.0;

                Vector3 hit_point = rec.point;
                Vector3 direction = current_ray.direction;

                current_ray = Ray(hit_point + direction * 0.001, direction);
                current_dist -= rec.t;
            } else {
                return 0.0;
            }
        } else {
            return transmission;
        }
    }
}


inline double calculate_shadow_factor(const Scene& scene, const HittableList& world, const Vector3& P, const Vector3& N) {
    if (!scene.shadows_enabled()) return 1.0;

    const int SHADOW_SAMPLES = 4;
    double shadow_factor = 0.0;

    for (const auto& light : scene.getLights()) {
        double light_shadow = 0.0;
        for (int i = 0; i < SHADOW_SAMPLES; i++) {
            Vector3 point_on_light = random_point_on_light(light);
            Vector3 shadow_ray_dir = point_on_light - P;
            double dist_to_light = shadow_ray_dir.length();
            shadow_ray_dir = shadow_ray_dir.normalize();

            Vector3 shadow_origin = P + N * 0.001;
            Ray shadow_ray(shadow_origin, shadow_ray_dir);

            // Accumulate transmission
            light_shadow += trace_shadow_transmission(shadow_ray, dist_to_light, world);
        }
        shadow_factor += (light_shadow / SHADOW_SAMPLES);
    }


    return 0;
}

inline double compute_light_visibility(const Scene& scene, const HittableList& world, const PointLight& light, const Vector3& P, const Vector3& N) {
    if (!scene.shadows_enabled()) return 1.0;

    int samples = scene.get_shadow_samples();

    double epsilon = Config::Instance().getDouble("advanced.epsilon", 1e-4);

    double shadow_accumulator = 0.0;

    for (int i = 0; i < samples; i++) {
        Vector3 point_on_light = random_point_on_light(light);
        Vector3 shadow_ray_dir = point_on_light - P;
        double dist_to_light = shadow_ray_dir.length();
        shadow_ray_dir = shadow_ray_dir.normalize();

        Vector3 shadow_origin = P + N * epsilon;
        Ray shadow_ray(shadow_origin, shadow_ray_dir);
        shadow_accumulator += trace_shadow_transmission(shadow_ray, dist_to_light, world);
    }
    return shadow_accumulator / samples;
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

    for (const auto& light : scene.getLights()) {
        double shadow_factor = compute_light_visibility(scene, world, light, P, N);

        if (shadow_factor > 0) {
            Vector3 L_raw = light.position - P;
            double dist_sq = L_raw.dot(L_raw);
            double falloff = 1.0 / dist_sq;
            Vector3 L = L_raw.normalize();

            Vector3 light_intensity = light.intensity * falloff * exposure;
            double L_dot_N = std::max(0.0, L.dot(N));

            Vector3 diffuse = component_wise_multiply(diffuse_colour, light_intensity) * L_dot_N * shadow_factor;
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
    const int SHADOW_SAMPLES = 4;

    for (const auto& light : scene.getLights()) {

        double shadow_factor = compute_light_visibility(scene, world, light, P, N);

        if (scene.shadows_enabled()) {
            shadow_factor = 0.0;
            for (int i = 0; i < SHADOW_SAMPLES; ++i) {

                Vector3 point_on_light = random_point_on_light(light);
                Vector3 shadow_ray_dir = point_on_light - P;
                double dist_to_light = shadow_ray_dir.length();
                shadow_ray_dir = shadow_ray_dir.normalize();

                Vector3 shadow_origin = P + N * 0.001;
                Ray shadow_ray(shadow_origin, shadow_ray_dir);
                shadow_factor += trace_shadow_transmission(shadow_ray, dist_to_light, world);
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