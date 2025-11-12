//
// Created by alex on 05/11/2025.
//

#ifndef B216602_TRACER_H
#define B216602_TRACER_H

#include "scene.h"
#include "ray.h"
#include "shapes/hittable.h"
#include "Image.h"
#include "shading.h"
#include "vector3.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

// prevents infinite recursion
const int MAX_RECURSION_DEPTH = 10;

// calculates the reflected ray direction
inline Vector3 reflect(const Vector3& V, const Vector3& N) {
    return V - 2 * V.dot(N) * N;
}

inline double random_double_tracer() {
    return rand() / (RAND_MAX + 1.0);
}

inline Vector3 random_in_unit_sphere_tracer() {
    while (true) {
        auto p = Vector3(random_double_tracer() * 2 - 1, random_double_tracer() * 2 - 1, random_double_tracer() * 2 - 1);
        if (p.dot(p) < 1.0)
            return p;
    }
}

inline double refract(double cos_i, double eta_ratio) {
    double r0 = (1.0 - eta_ratio) / (1.0 + eta_ratio);
    r0 = r0 * r0;

    double n_val = eta_ratio > 1.0 ? eta_ratio : 1.0 / eta_ratio;
    double r_n = (n_val - 1.0) / (n_val + 1.0);
    r0 = r_n * r_n;

    return r0 + (1.0 - r0) * pow((1.0 - cos_i), 5.0);
}

inline Vector3 ray_colour(const Ray& r, const Scene& scene, const HittableList& world, int depth) {
    if (depth <= 0) {
        return Vector3(0, 0, 0); // Return black
    }
    HitRecord rec;
    Vector3 background_colour_vec(0.5, 0.7, 1.0);

    // if the ray hits an object
    if (world.intersect(r, 0.001, 100000.0, rec)) {
        Vector3 local_ad_colour = calculate_local_ad(rec, scene, world); // ad = ambient diffuse
        Vector3 reflected_colour(0, 0, 0);
        Vector3 refracted_colour(0, 0, 0);
        Vector3 final_colour(0, 0, 0);

        if (scene.get_glossy_samples() > 0) {
            if (rec.mat.reflectivity > 0) {
                double roughness = 1.0 / sqrt(rec.mat.shininess);
                roughness = std::min(roughness, 1.0);

                Vector3 V = (r.origin - rec.point).normalize();
                Vector3 perfect_reflect_dir = reflect(-V, rec.normal).normalize();

                const int GLOSSY_SAMPLES = scene.get_glossy_samples();
                for (int i = 0; i < GLOSSY_SAMPLES; i++) {
                    Vector3 random_offset = random_in_unit_sphere_tracer() * roughness;
                    Vector3 glossy_dir = (perfect_reflect_dir + random_offset).normalize();
                    if (glossy_dir.dot(rec.normal) > 0) {
                        Ray reflect_ray(rec.point, glossy_dir, r.time);
                        reflected_colour = reflected_colour + ray_colour(reflect_ray, scene, world, depth - 1);
                    }
                }
                reflected_colour = reflected_colour * (1.0 / GLOSSY_SAMPLES);
            }
            if (rec.mat.transparency > 0) {
                Vector3 V_in = r.direction.normalize();
                Vector3 N_hit = rec.normal.normalize();
                bool entering = V_in.dot(N_hit) < 0;
                Vector3 N_outward = entering ? N_hit : -N_hit;
                double n1 = entering ? 1.0 : rec.mat.refractive_index;
                double n2 = entering ? rec.mat.refractive_index : 1.0;
                double eta_ratio = n1/n2;

                double cos_i = -V_in.dot(N_outward);
                double sin_t_squared = eta_ratio * eta_ratio * (1.0 - cos_i * cos_i);

                if (sin_t_squared <= 1.0) {
                    double cos_t = sqrt(1.0 - sin_t_squared);
                    Vector3 refract_dir = (eta_ratio * V_in) + (eta_ratio * cos_i - cos_t) * N_outward;

                    Ray refract_ray(rec.point, refract_dir.normalize(), r.time);
                    refracted_colour = ray_colour(refract_ray, scene, world, depth - 1);
                } else {
                    Vector3 V = r.direction.normalize();
                    Vector3 reflect_dir = reflect(V, rec.normal).normalize();
                    Ray reflect_ray(rec.point, reflect_dir, r.time);
                    refracted_colour = ray_colour(reflect_ray, scene, world, depth - 1);
                }
            }
            final_colour = local_ad_colour * (1.0 - rec.mat.reflectivity - rec.mat.transparency)
                            + reflected_colour * rec.mat.reflectivity
                            + refracted_colour * rec.mat.transparency;
        }
        else {
            Vector3 specular_colour = calculate_specular(rec, scene, world, r);
            // calculate reflected colour
            if (rec.mat.reflectivity > 0) {
                Vector3 V = (r.origin - rec.point).normalize();
                Vector3 reflect_dir = reflect(-V, rec.normal).normalize();

                Ray reflect_ray(rec.point, reflect_dir, r.time);

                reflected_colour = ray_colour(reflect_ray, scene, world, depth - 1);
            }
            if (rec.mat.transparency > 0) {
                Vector3 V_in = r.direction.normalize();
                Vector3 N_hit = rec.normal.normalize();
                bool entering = V_in.dot(N_hit) < 0;
                Vector3 N_outward = entering ? N_hit : -N_hit;
                double n1 = entering ? 1.0 : rec.mat.refractive_index;
                double n2 = entering ? rec.mat.refractive_index : 1.0;
                double eta_ratio = n1/n2;

                double cos_i = -V_in.dot(N_outward);
                double sin_t_squared = eta_ratio * eta_ratio * (1.0 - cos_i * cos_i);

                if (sin_t_squared <= 1.0) {
                    double cos_t = sqrt(1.0 - sin_t_squared);
                    Vector3 refract_dir = (eta_ratio * V_in) + (eta_ratio * cos_i - cos_t) * N_outward;

                    Ray refract_ray(rec.point, refract_dir.normalize(), r.time);
                    refracted_colour = ray_colour(refract_ray, scene, world, depth - 1);
                } else {
                    Vector3 V = r.direction.normalize();
                    Vector3 reflect_dir = reflect(V, rec.normal).normalize();
                    Ray reflect_ray(rec.point, reflect_dir, r.time);
                    refracted_colour = ray_colour(reflect_ray, scene, world, depth - 1);
                }
            }
            final_colour = local_ad_colour * (1.0 - rec.mat.reflectivity - rec.mat.transparency)
                                    + reflected_colour * rec.mat.reflectivity
                                    + refracted_colour * rec.mat.transparency
                                    + specular_colour; // manually added specular_colour in, artistic choice
        }
        return final_colour;
    } else {
        return background_colour_vec;
    }
}

inline Pixel final_colour_to_pixel(const Vector3& colour_vec) {
    auto clamp = [](double val) {
        return std::max(0.0, std::min(1.0, val));
    };
    unsigned char r = static_cast<unsigned char>(255 * clamp(colour_vec.x));
    unsigned char g = static_cast<unsigned char>(255 * clamp(colour_vec.y));
    unsigned char b = static_cast<unsigned char>(255 * clamp(colour_vec.z));
    return {r, g, b};
}
#endif //B216602_TRACER_H