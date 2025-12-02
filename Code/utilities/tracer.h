//
// Created by alex on 05/11/2025.
//

#ifndef B216602_TRACER_H
#define B216602_TRACER_H

#include "scene.h"
#include "ray.h"
#include "../shapes/hittable.h"
#include "Image.h"
#include "shading.h"
#include "vector3.h"
#include "../environment/HDRImage.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include "random_utils.h"
#include "../config.h"

// Reinhardt Tone Mapping
// Formula: C / (1 + C)
inline Vector3 tonemap_reinhard(const Vector3& v) {
    return Vector3(v.x / (1.0 + v.x), v.y / (1.0 + v.y), v.z / (1.0 + v.z));
}

// ACES
inline Vector3 tonemap_aces(const Vector3& v) {
    auto aces = [](double x) {
        const double a = 2.51;
        const double b = 0.03;
        const double c = 2.43;
        const double d = 0.59;
        const double e = 0.14;
        return (x * (a * x + b)) / (x * (c * x + d) + e);
    };
    return Vector3(aces(v.x), aces(v.y), aces(v.z));
}

// Filmic
inline Vector3 tonemap_filmic(const Vector3& v) {
    auto filmic = [](double x) {
        const double A = 0.15;
        const double B = 0.50;
        const double C = 0.10;
        const double D = 0.20;
        const double E = 0.02;
        const double F = 0.30;
        return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
    };
    const double W = 11.2; // Linear White Point Value
    double white_scale = 1.0 / filmic(W);
    return Vector3(filmic(v.x) * white_scale, filmic(v.y) * white_scale, filmic(v.z) * white_scale);
}

// calculates the reflection of a vector v about a normal n.
inline Vector3 reflect(const Vector3& V, const Vector3& N) {
    // calculates the reflection vector using the formula: r = v - 2 * dot(v, n) * n.
    return V - 2 * V.dot(N) * N;
}

// calculates the spherical (u, v) coordinates for a point p on a unit sphere.
inline void get_sphere_uv(const Vector3& p, double& u, double& v) {

    // maps the point's coordinates to directions. assumes z is up.
    double d_up = p.z;
    double d_fwd = p.y;
    double d_right = p.x;

    // assumes z is up for the sphere mapping.
    // u corresponds to longitude (horizontal angle).
    // v corresponds to latitude (vertical angle).
    // calculates the longitude (azimuthal angle) from the x and y components.
    auto longitude = atan2(d_fwd, d_right);
    // calculates the latitude (polar angle) from the z component.
    auto latitude = acos(d_up);

    // assign the calculated longitude to u.
    u = longitude;
    // flip the latitude to match common texture mapping conventions (origin at top-left).
    v = M_PI - latitude;
}




// approximates the fresnel factor using schlick's approximation.
inline double schlick(double cos_i, double n1, double n2) {
    // calculates reflectance at normal incidence using the formula: r0 = ((n1 - n2) / (n1 + n2))^2.
    double r0 = ((n1 - n2) / (n1 + n2)) * ((n1 - n2) / (n1 + n2));
    // intermediate term for the approximation.
    double x = 1.0 - cos_i;
    // calculates (1 - cos(i))^5.
    double x5 = x * x * x * x * x;
    // calculates the schlick approximation using the formula: r(θ) = r0 + (1 - r0) * (1 - cos(θ))^5.
    return r0 + (1.0 - r0) * x5;
}

// computes the refracted ray direction and the fresnel reflection probability.
inline bool compute_refraction(const Vector3& V_in, const Vector3& N, double refractive_index,
                             bool front_face, Vector3& refract_dir, double& out_reflect_prob, bool fresnel_enabled) {

    double n1, n2;
    Vector3 N_outward = N;

    if (front_face) {
        // ray is entering the object from air.
        n1 = 1.0;
        n2 = refractive_index;
    } else {
        n1 = refractive_index;
        n2 = 1.0;
    }

    // calculates the ratio of refractive indices, η = n1 / n2.
    double eta_ratio = n1 / n2;
    // calculates the cosine of the angle of incidence.
    double cos_i = -V_in.dot(N_outward);
    // calculates sin^2(θt) using snell's law: sin^2(θt) = η^2 * (1 - cos^2(θi)).
    double sin_t_squared = eta_ratio * eta_ratio * (1.0 - cos_i * cos_i);

    // checks for total internal reflection.
    if (sin_t_squared > 1.0) {
        // total internal reflection occurs, so reflection probability is 1.
        out_reflect_prob = 1.0;
        return false;
    }

    // calculates the cosine of the transmission angle: cos(θt) = sqrt(1 - sin^2(θt)).
    double cos_t = sqrt(1.0 - sin_t_squared);
    // calculates the refracted ray direction using the vector form of snell's law.
    refract_dir = (eta_ratio * V_in) + (eta_ratio * cos_i - cos_t) * N_outward;

    // calculates the reflection probability using the fresnel effect if enabled.
    if (fresnel_enabled) {
        // uses schlick's approximation to determine reflection probability.
        out_reflect_prob = schlick(cos_i, n1, n2);
    } else {
        out_reflect_prob = 0.0; // Will rely purely on material transparency property
    }

    return true;
}


// recursively traces a ray and calculates the color seen along its path.
inline Vector3 ray_colour(const Ray& r, const Scene& scene, const HittableList& world, int depth) {
    // stops recursion if the maximum depth is reached.
    if (depth <= 0) return Vector3(0, 0, 0);

    // gets a small offset value to prevent self-intersection artifacts.
    double epsilon = scene.get_epsilon();

    HitRecord rec;
    // checks if the ray intersects with any object in the world.
    if (world.intersect(r, epsilon, 100000.0, rec)) {
        if (scene.rendering_normals()) {
            // Map Normal [-1, 1] to Colour [0, 1]
            // Equation: Colour = 0.5 * (Normal + 1.0)
            return (rec.normal + Vector3(1, 1, 1)) * 0.5;
        }

        Vector3 diffuse_ambient = calculate_local_ad(rec, scene, world, r.time);
        Vector3 specular_highlight = calculate_specular(rec, scene, world, r);

        // initializes reflected and refracted color components.
        Vector3 reflected_colour(0, 0, 0);
        Vector3 refracted_colour(0, 0, 0);

        // determines if the material is transparent or reflective.
        bool is_transparent = rec.mat.transparency > 0;
        bool has_reflection = rec.mat.reflectivity > 0;

        // if fresnel is enabled, any transparent object can also be reflective.
        if (is_transparent && scene.fresnel_enabled()) has_reflection = true;
        if (has_reflection) {
            // gets the number of samples for glossy reflections.
            // only uses glossy for a max number of bounces to reduce computation.
            int samples = scene.get_glossy_samples();
            if (depth < scene.get_max_bounces()) {
                samples = 1;
            }
            // calculates roughness from shininess for glossy reflections.
            double roughness = 1.0 / sqrt(rec.mat.shininess);

            // gets the normalized incoming ray direction.
            Vector3 V = r.direction.normalize();
            // calculates the perfect reflection direction.
            Vector3 perfect_reflect_dir = reflect(V, rec.normal).normalize();

            // handles glossy reflections with multiple samples.
            if (samples > 0) {
                for (int i = 0; i < samples; i++) {
                    // creates a random offset in a unit sphere, scaled by roughness.
                    Vector3 random_offset = random_in_unit_sphere() * roughness;
                    // calculates a perturbed reflection direction for a glossy effect.
                    Vector3 target_dir = (perfect_reflect_dir + random_offset).normalize();
                    // ensures the reflected ray is on the same side of the surface as the normal.
                    if (target_dir.dot(rec.normal) > 0) {
                        // creates the reflected ray, offset slightly to avoid self-intersection.
                        Ray reflect_ray(rec.point + rec.normal * epsilon, target_dir, r.time);
                        // recursively traces the reflected ray and accumulates color.
                        reflected_colour = reflected_colour + ray_colour(reflect_ray, scene, world, depth - 1);
                    }
                }
                // averages the color from all glossy samples.
                reflected_colour = reflected_colour * (1.0 / samples);
            } else {
                // handles perfect (mirror) reflection with a single ray.
                Ray reflect_ray(rec.point + rec.normal * epsilon, perfect_reflect_dir, r.time);
                // recursively traces the reflected ray.
                reflected_colour = ray_colour(reflect_ray, scene, world, depth - 1);
            }

            // for metal materials, the reflected color is tinted by the material's diffuse color.
            if (rec.mat.type == "metal") {
                reflected_colour = component_wise_multiply(reflected_colour, rec.mat.diffuse);
            }
        }

        // gets the base reflection and transmission probabilities from the material.
        double reflect_prob = rec.mat.reflectivity;
        double transmit_prob = rec.mat.transparency;

        // handles refraction for transparent materials.
        if (is_transparent) {
            Vector3 refract_dir;
            double fresnel_reflect_prob = 0.0;
            // gets the normalized incoming ray direction and hit normal.
            Vector3 V_in = r.direction.normalize();
            Vector3 N_hit = rec.normal.normalize();

            // computes the refraction direction and fresnel reflection probability.
            bool valid_refraction = compute_refraction(V_in, N_hit, rec.mat.refractive_index,
                                                     rec.front_face, refract_dir, fresnel_reflect_prob,
                                                     scene.fresnel_enabled());

            // if refraction is possible (no total internal reflection).
            if (valid_refraction) {
                // creates the refracted ray.
                Ray refract_ray(rec.point, refract_dir.normalize(), r.time);
                // recursively traces the refracted ray.
                refracted_colour = ray_colour(refract_ray, scene, world, depth - 1);
                // tints the refracted color by the material's diffuse color (like colored glass).
                refracted_colour = component_wise_multiply(refracted_colour, rec.mat.diffuse);

                // if fresnel is enabled, update reflection and transmission probabilities.
                if (scene.fresnel_enabled()) {
                    reflect_prob = fresnel_reflect_prob;
                    // transmission probability is the remainder.
                    transmit_prob = 1.0 - reflect_prob;
                }
            } else {
                // total internal reflection occurred.
                transmit_prob = 0.0;
                reflect_prob = 1.0;
                // if reflection wasn't already calculated, calculate it now.
                if (!has_reflection) {
                    // calculates the reflection direction.
                    Vector3 v_reflect = reflect(V_in, N_hit).normalize();
                    // creates and traces the reflected ray.
                    Ray reflect_ray(rec.point + N_hit * epsilon, v_reflect, r.time);
                    reflected_colour = ray_colour(reflect_ray, scene, world, depth - 1);
                }
            }
        }
        // combines local, reflected, and refracted colors for transparent materials.
        if (is_transparent) {
            // final color is a mix based on reflection and transmission probabilities.
            return (reflected_colour * reflect_prob)
                 + (refracted_colour * transmit_prob)
                 + specular_highlight;
        } else {
            // Opaque/Metal
            return diffuse_ambient * (1.0 - rec.mat.reflectivity)
                 + reflected_colour * rec.mat.reflectivity
                 + specular_highlight;

        }
    } else {
        // if the ray doesn't hit any object, sample the background.
        if (scene.has_hdr_background()) {
            double u, v;
            // converts the ray direction to spherical coordinates.
            get_sphere_uv(r.direction.normalize(), u, v);
            // samples the hdr image at the calculated (u, v) coordinates.
            return scene.get_hdr_background()->sample(u,v);
        }

        double bg_r = Config::Instance().getDouble("background.r", 0.5);
        double bg_g = Config::Instance().getDouble("background.g", 0.7);
        double bg_b = Config::Instance().getDouble("background.b", 1.0);
        return Vector3(bg_r, bg_g, bg_b); // default background
    }
}

inline Pixel final_colour_to_pixel(const Vector3& colour_vec) {
    // a lambda function to clamp a value between 0.0 and 1.0.
    auto clamp = [](double val) { return std::max(0.0, std::min(1.0, val)); };
    return {
        // clamps and scales the red component to an 8-bit unsigned char.
        static_cast<unsigned char>(255 * clamp(colour_vec.x)),
        // clamps and scales the green component to an 8-bit unsigned char.
        static_cast<unsigned char>(255 * clamp(colour_vec.y)),
        // clamps and scales the blue component to an 8-bit unsigned char.
        static_cast<unsigned char>(255 * clamp(colour_vec.z))
    };
}
#endif //B216602_TRACER_H