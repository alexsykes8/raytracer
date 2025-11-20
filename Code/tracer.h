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
#include "HDRImage.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include "random_utils.h"
#include "config.h"


// calculates the reflected ray direction
inline Vector3 reflect(const Vector3& V, const Vector3& N) {
    return V - 2 * V.dot(N) * N;
}


inline void get_sphere_uv(const Vector3& p, double& u, double& v) {

    double d_up = p.z;
    double d_fwd = p.y;
    double d_right = p.x;

    // Assumes Y is up.
    // u is longitude (horizontal)
    // v is latitude (vertical)
    // traces ray to a long/lat coord on the sphere
    auto longitude = atan2(d_fwd, d_right);
    auto latitude = acos(d_up);

    u = longitude;
    // flip vertically
    v = M_PI - latitude;
}




// used to approximate the contribution of the fresnel factor in the specular reflection of light
inline double schlick(double cos_i, double n1, double n2) {
    double r0 = ((n1 - n2) / (n1 + n2)) * ((n1 - n2) / (n1 + n2));

    // direct multiplication used instead of pow for x^5, faster
    double x = 1.0 - cos_i;
    double x2 = x * x;
    return r0 + (1.0 - r0) * (x2 * x2 * x);
}

inline bool compute_refraction(const Vector3& V_in, const Vector3& N, double refractive_index,
                             bool front_face, Vector3& refract_dir, double& out_reflect_prob, bool fresnel_enabled) {

    double n1, n2;
    Vector3 N_outward = N;

    if (front_face) {
        n1 = 1.0;
        n2 = refractive_index;
    } else {
        n1 = refractive_index;
        n2 = 1.0;
    }

    double eta_ratio = n1 / n2;
    double cos_i = -V_in.dot(N_outward);
    cos_i = std::clamp(cos_i, 0.0, 1.0);
    double sin_t_squared = eta_ratio * eta_ratio * (1.0 - cos_i * cos_i);

    // Check for Total Internal Reflection
    if (sin_t_squared > 1.0) {
        out_reflect_prob = 1.0; // Full reflection
        return false;
    }

    // Calculate Refraction Direction
    double cos_t = sqrt(1.0 - sin_t_squared);
    refract_dir = (eta_ratio * V_in) + (eta_ratio * cos_i - cos_t) * N_outward;

    // Calculate Fresnel Effect if enabled
    if (fresnel_enabled) {
        double cos_theta = cos_i;
        // If moving to a denser medium, use transmitted angle for Schlick
        if (n1 > n2) {
            cos_theta = cos_t;
        }
        out_reflect_prob = schlick(cos_theta, n1, n2);
    } else {
        out_reflect_prob = 0.0; // Will rely purely on material transparency property
    }

    return true;
}


// provided a scene and the objects in it, along with a ray to trace.
inline Vector3 ray_colour(const Ray& r, const Scene& scene, const HittableList& world, int depth) {
    if (depth <= 0) return Vector3(0, 0, 0);

    double epsilon = scene.get_epsilon();

    HitRecord rec;
    if (world.intersect(r, epsilon, 100000.0, rec)) {
        Vector3 local_colour = calculate_local_ad(rec, scene, world) +
                               calculate_specular(rec, scene, world, r);

        Vector3 reflected_colour(0, 0, 0);
        Vector3 refracted_colour(0, 0, 0);

        bool is_transparent = rec.mat.transparency > 0;
        bool has_reflection = rec.mat.reflectivity > 0;

        if (is_transparent && scene.fresnel_enabled()) has_reflection = true;
        if (has_reflection) {
            int samples = scene.get_glossy_samples();
            int loop_count = (samples > 0) ? samples : 1;

            double roughness = 1.0 / sqrt(rec.mat.shininess);
            roughness = std::min(roughness, 1.0);

            Vector3 V = r.direction.normalize();
            Vector3 perfect_reflect_dir = reflect(V, rec.normal).normalize();

            for (int i = 0; i < loop_count; i++) {
                Vector3 target_dir = perfect_reflect_dir;
                if (samples > 0) {
                    Vector3 random_offset = random_in_unit_sphere() * roughness;
                    target_dir = (perfect_reflect_dir + random_offset).normalize();
                }

                if (target_dir.dot(rec.normal) > 0) { // Ensure ray is not pointing into the surface
                    Ray reflect_ray(rec.point + rec.normal * epsilon, target_dir, r.time);
                    reflected_colour = reflected_colour + ray_colour(reflect_ray, scene, world, depth - 1);
                }
            }
            reflected_colour = reflected_colour * (1.0 / loop_count);
            if (rec.mat.type == "metal") {
                reflected_colour = component_wise_multiply(reflected_colour, rec.mat.diffuse);
            }

        }

        double reflect_prob = rec.mat.reflectivity;
        double transmit_prob = rec.mat.transparency;

        if (is_transparent) {
            Vector3 refract_dir;
            double fresnel_reflect_prob = 0.0;
            Vector3 V_in = r.direction.normalize();
            Vector3 N_hit = rec.normal.normalize();

            bool valid_refraction = compute_refraction(V_in, N_hit, rec.mat.refractive_index,
                                                     rec.front_face, refract_dir, fresnel_reflect_prob,
                                                     scene.fresnel_enabled());

            if (valid_refraction) {
                Ray refract_ray(rec.point, refract_dir.normalize(), r.time);
                refracted_colour = ray_colour(refract_ray, scene, world, depth - 1);

                refracted_colour = component_wise_multiply(refracted_colour, rec.mat.diffuse);

                if (scene.fresnel_enabled()) {
                    reflect_prob = fresnel_reflect_prob;
                    transmit_prob = 1.0 - reflect_prob;
                }
            } else {
                transmit_prob = 0.0;
                reflect_prob = 1.0;
                if (!has_reflection) {
                    Vector3 v_reflect = reflect(V_in, N_hit).normalize();
                    Ray reflect_ray(rec.point + N_hit * epsilon, v_reflect, r.time);
                    reflected_colour = ray_colour(reflect_ray, scene, world, depth - 1);
                }
            }
        }
        if (is_transparent) {
            return local_colour * (1.0 - reflect_prob - transmit_prob)
                 + reflected_colour * reflect_prob
                 + refracted_colour * transmit_prob;
        } else {
            return local_colour * (1.0 - rec.mat.reflectivity)
                 + reflected_colour * rec.mat.reflectivity;
        }
    } else {
        if (scene.has_hdr_background()) {
            double u, v;
            get_sphere_uv(r.direction.normalize(), u, v);
            return scene.get_hdr_background()->sample(u,v);
        }
        return Vector3(0.5, 0.7, 1.0); // default background
    }
}

inline Pixel final_colour_to_pixel(const Vector3& colour_vec) {
    auto clamp = [](double val) { return std::max(0.0, std::min(1.0, val)); };
    return {
        static_cast<unsigned char>(255 * clamp(colour_vec.x)),
        static_cast<unsigned char>(255 * clamp(colour_vec.y)),
        static_cast<unsigned char>(255 * clamp(colour_vec.z))
    };
}
#endif //B216602_TRACER_H