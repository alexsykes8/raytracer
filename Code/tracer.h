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


// used to approximate the contribution of the fresnel factor in the specular reflection of light
inline double schlick(double cos_i, double n1, double n2) {
    double r0 = ((n1 - n2) / (n1 + n2)) * ((n1 - n2) / (n1 + n2));

    return r0 + (1.0 - r0) * pow((1.0 - cos_i), 5.0);
}

// provided a scene and the objects in it, along with a ray to trace.
inline Vector3 ray_colour(const Ray& r, const Scene& scene, const HittableList& world, int depth) {
    if (depth <= 0) {
        return Vector3(0, 0, 0); // Return black
    }
    HitRecord rec;
    Vector3 background_colour_vec(0.5, 0.7, 1.0);

    // if the ray hits an object.
    if (world.intersect(r, 0.001, 100000.0, rec)) {
        // This takes a point that was hit by a ray from the camera (guaranteed to be visible) and finds its colour based on lights in the scene.
        Vector3 local_ad_colour = calculate_local_ad(rec, scene, world); // ad = ambient diffuse
        Vector3 reflected_colour(0, 0, 0);
        Vector3 refracted_colour(0, 0, 0);
        Vector3 final_colour(0, 0, 0);

        // if the glossy flag is present, use Monte Carlo path tracing to calculate the reflection to simulate rough surfaces. This is instead of using one perfectly sharp reflected ray like a mirror.
        if (scene.get_glossy_samples() > 0) {
            if (rec.mat.reflectivity > 0) {
                // the shininess property is inversely related to roughness.
                double roughness = 1.0 / sqrt(rec.mat.shininess);
                roughness = std::min(roughness, 1.0);

                // V is the rays incoming direction.
                Vector3 V = (r.direction).normalize();
                // reflect calculates the ideal mirror-like reflection vector based on the surfaces normal and the incoming ray direction.
                Vector3 perfect_reflect_dir = reflect(V, rec.normal).normalize();

                const int GLOSSY_SAMPLES = scene.get_glossy_samples();
                // for each sample
                for (int i = 0; i < GLOSSY_SAMPLES; i++) {
                    // Depending on the roughness, generates a random 3D vector, scaled by roughness.
                    Vector3 random_offset = random_in_unit_sphere_tracer() * roughness;
                    // Add this random vector to the reflection direction to create a scattered reflection (cone of rays)
                    Vector3 glossy_dir = (perfect_reflect_dir + random_offset).normalize();
                    // if the scattered ray is pointing above the surface (not into the object)
                    if (glossy_dir.dot(rec.normal) > 0) {
                        // a new ray is created with the scattered direction and recursively traced to find the colour contribution from the environment
                        Ray reflect_ray(rec.point, glossy_dir, r.time);
                        // accumulate the colour of each scattered ray.
                        reflected_colour = reflected_colour + ray_colour(reflect_ray, scene, world, depth - 1);
                    }
                }
                // average the accumulated colours.
                reflected_colour = reflected_colour * (1.0 / GLOSSY_SAMPLES);
            }
            // if the object is transparent
            // uses Snell's Law n1 sin(theta1) = n2 sin(theta2)
            if (rec.mat.transparency > 0) {
                // prepares the incoming ray
                Vector3 V_in = r.direction.normalize();
                // prepares the surface normal
                Vector3 N_hit = rec.normal.normalize();
                // checks if the incoming ray is entering the surface. If the dot product is negative, the ray is entering
                bool entering = V_in.dot(N_hit) < 0;
                // sets the outward pointing normal. It must always point against the incoming ray
                Vector3 N_outward;
                // the refractive indices between air and the object. Air is set to 1.0. If entering, then n1 is the air, if exiting then n2 is the air.
                double n1;
                double n2;
                if (entering) {
                    N_outward = N_hit;
                    n1 = 1.0;
                    n2 = rec.mat.refractive_index;
                } else {
                    N_outward = -N_hit;
                    n1 = rec.mat.refractive_index;
                    n2 = 1.0;
                }

                // ratio of the refractive indices
                double eta_ratio = n1/n2;

                // Checking for total internal reflection: this determines if refraction is mathematically possible.
                // If the second medium has a lower refractive index that the first, such as light moving from water to air, then it is possible for TIR to occur.
                // If TIR occurs, a ray will reflect internal instead of leaving the medium.

                // calculates the cosine of the angle of incidence.
                double cos_i = -V_in.dot(N_outward);
                // calculates the squared sine of the angle of incidence.
                // sin(theta_2) = eta_ratio * sin(theta_1) = eta_ratio * sqrt((1-cos(theta_1)^2))
                double sin_t_squared = eta_ratio * eta_ratio * (1.0 - cos_i * cos_i);

                // if sin^2 theta_t is less than or equal to 1, the ray successfully passed between the two mediums (no TIR).
                if (sin_t_squared <= 1.0) {
                    // basic trig
                    double cos_t = sqrt(1.0 - sin_t_squared);
                    // refraction direction.
                    // https://tinyurl.com/4hvxv2xb
                    Vector3 refract_dir = (eta_ratio * V_in) + (eta_ratio * cos_i - cos_t) * N_outward;

                    // a recursive ray is case to find the colour transmitted through the object and into the environment
                    Ray refract_ray(rec.point, refract_dir.normalize(), r.time);
                    refracted_colour = ray_colour(refract_ray, scene, world, depth - 1);
                } else { // if sin^2 theta_t is greater than 1.0, the ray is hitting the surface at a shallow angle and total internal reflection occurs.
                    Vector3 V = r.direction.normalize();
                    // the ray falls back to calculating a mirror-like reflection.
                    Vector3 reflect_dir = reflect(V, rec.normal).normalize();
                    Ray reflect_ray(rec.point, reflect_dir, r.time);
                    // a recursive ray is cast along the reflection vector.
                    refracted_colour = ray_colour(reflect_ray, scene, world, depth - 1);
                }
            }

            double reflect_prob = rec.mat.reflectivity;
            double transmit_prob = rec.mat.transparency;

            if (rec.mat.transparency > 0 && scene.fresnel_enabled()) {

                Vector3 V_in = r.direction.normalize();
                Vector3 N_hit = rec.normal.normalize();
                bool entering = V_in.dot(N_hit) < 0;
                Vector3 N_outward;
                double n1;
                double n2;
                if (entering) {
                    N_outward = N_hit;
                    n1 = 1.0;
                    n2 = rec.mat.refractive_index;
                } else {
                    N_outward = -N_hit;
                    n1 = rec.mat.refractive_index;
                    n2 = 1.0;
                }
                double eta_ratio = n1/n2;
                double cos_i = -V_in.dot(N_outward);
                double sin_t_squared = eta_ratio * eta_ratio * (1.0 - cos_i * cos_i);

                // approximate the contribution of the fresnel factor in the specular reflection of light
                reflect_prob = schlick(cos_i, n1, n2);

                // TIR
                if (sin_t_squared > 1.0) {
                    transmit_prob = 0.0;
                    reflect_prob = 1.0;
                }
                // NO TIR but some amount of reflection.
                else {
                    transmit_prob = 1.0 - reflect_prob;
                }
            }

            if (rec.mat.transparency > 0) {
                final_colour = local_ad_colour * (1.0 - reflect_prob - transmit_prob)
                            + reflected_colour * reflect_prob
                            + refracted_colour * transmit_prob;
            } else {
                final_colour = local_ad_colour * (1.0 - rec.mat.reflectivity - rec.mat.transparency)
                            + reflected_colour * rec.mat.reflectivity
                            + refracted_colour * rec.mat.transparency;
            }
        }
        else {
            if (rec.mat.reflectivity > 0) {
                // the shininess property is inversely related to roughness.
                double roughness = 1.0 / sqrt(rec.mat.shininess);
                roughness = std::min(roughness, 1.0);

                // V is the rays incoming direction.
                Vector3 V = (r.direction).normalize();
                // reflect calculates the ideal mirror-like reflection vector based on the surfaces normal and the incoming ray direction.
                Vector3 perfect_reflect_dir = reflect(V, rec.normal).normalize();

                // Depending on the roughness, generates a random 3D vector, scaled by roughness.
                Vector3 random_offset = random_in_unit_sphere_tracer() * roughness;
                // Add this random vector to the reflection direction to create a scattered reflection (cone of rays)
                Vector3 glossy_dir = (perfect_reflect_dir + random_offset).normalize();
                // if the scattered ray is pointing above the surface (not into the object)
                if (glossy_dir.dot(rec.normal) > 0) {
                    // a new ray is created with the scattered direction and recursively traced to find the colour contribution from the environment
                    Ray reflect_ray(rec.point, glossy_dir, r.time);
                    // accumulate the colour of each scattered ray.
                    reflected_colour = reflected_colour + ray_colour(reflect_ray, scene, world, depth - 1);
                }
            }
            // if the object is transparent
            // uses Snell's Law n1 sin(theta1) = n2 sin(theta2)
            if (rec.mat.transparency > 0) {
                // prepares the incoming ray
                Vector3 V_in = r.direction.normalize();
                // prepares the surface normal
                Vector3 N_hit = rec.normal.normalize();
                // checks if the incoming ray is entering the surface. If the dot product is negative, the ray is entering
                bool entering = V_in.dot(N_hit) < 0;
                // sets the outward pointing normal. It must always point against the incoming ray
                Vector3 N_outward;
                // the refractive indices between air and the object. Air is set to 1.0. If entering, then n1 is the air, if exiting then n2 is the air.
                double n1;
                double n2;
                if (entering) {
                    N_outward = N_hit;
                    n1 = 1.0;
                    n2 = rec.mat.refractive_index;
                } else {
                    N_outward = -N_hit;
                    n1 = rec.mat.refractive_index;
                    n2 = 1.0;
                }

                // ratio of the refractive indices
                double eta_ratio = n1/n2;

                // Checking for total internal reflection: this determines if refraction is mathematically possible.
                // If the second medium has a lower refractive index that the first, such as light moving from water to air, then it is possible for TIR to occur.
                // If TIR occurs, a ray will reflect internal instead of leaving the medium.

                // calculates the cosine of the angle of incidence.
                double cos_i = -V_in.dot(N_outward);
                // calculates the squared sine of the angle of incidence.
                // sin(theta_2) = eta_ratio * sin(theta_1) = eta_ratio * sqrt((1-cos(theta_1)^2))
                double sin_t_squared = eta_ratio * eta_ratio * (1.0 - cos_i * cos_i);

                // if sin^2 theta_t is less than or equal to 1, the ray successfully passed between the two mediums (no TIR).
                if (sin_t_squared <= 1.0) {
                    // basic trig
                    double cos_t = sqrt(1.0 - sin_t_squared);
                    // refraction direction.
                    // https://tinyurl.com/4hvxv2xb
                    Vector3 refract_dir = (eta_ratio * V_in) + (eta_ratio * cos_i - cos_t) * N_outward;

                    // a recursive ray is case to find the colour transmitted through the object and into the environment
                    Ray refract_ray(rec.point, refract_dir.normalize(), r.time);
                    refracted_colour = ray_colour(refract_ray, scene, world, depth - 1);
                } else { // if sin^2 theta_t is greater than 1.0, the ray is hitting the surface at a shallow angle and total internal reflection occurs.
                    Vector3 V = r.direction.normalize();
                    // the ray falls back to calculating a mirror-like reflection.
                    Vector3 reflect_dir = reflect(V, rec.normal).normalize();
                    Ray reflect_ray(rec.point, reflect_dir, r.time);
                    // a recursive ray is cast along the reflection vector.
                    refracted_colour = ray_colour(reflect_ray, scene, world, depth - 1);
                }
            }

            double reflect_prob = rec.mat.reflectivity;
            double transmit_prob = rec.mat.transparency;

            if (rec.mat.transparency > 0 && scene.fresnel_enabled()) {

                Vector3 V_in = r.direction.normalize();
                Vector3 N_hit = rec.normal.normalize();
                bool entering = V_in.dot(N_hit) < 0;
                Vector3 N_outward;
                double n1;
                double n2;
                if (entering) {
                    N_outward = N_hit;
                    n1 = 1.0;
                    n2 = rec.mat.refractive_index;
                } else {
                    N_outward = -N_hit;
                    n1 = rec.mat.refractive_index;
                    n2 = 1.0;
                }
                double eta_ratio = n1/n2;
                double cos_i = -V_in.dot(N_outward);
                double sin_t_squared = eta_ratio * eta_ratio * (1.0 - cos_i * cos_i);

                // approximate the contribution of the fresnel factor in the specular reflection of light
                reflect_prob = schlick(cos_i, n1, n2);

                // TIR
                if (sin_t_squared > 1.0) {
                    transmit_prob = 0.0;
                    reflect_prob = 1.0;
                }
                // NO TIR but some amount of reflection.
                else {
                    transmit_prob = 1.0 - reflect_prob;
                }
            }

            if (rec.mat.transparency > 0) {
                final_colour = local_ad_colour * (1.0 - reflect_prob - transmit_prob)
                            + reflected_colour * reflect_prob
                            + refracted_colour * transmit_prob;
            } else {
                final_colour = local_ad_colour * (1.0 - rec.mat.reflectivity - rec.mat.transparency)
                            + reflected_colour * rec.mat.reflectivity
                            + refracted_colour * rec.mat.transparency;
            }
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