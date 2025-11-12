//
// Created by alex on 12/11/2025.
//

#ifndef B216602_TRACER_H
#define B216602_TRACER_H

#include "scene.h"
#include "ray.h"
#include "shapes/hittable.h"
#include "shading.h"
#include "vector3.h"
#include <cmath>

const int MAX_RECURSION_DEPTH = 10;

inline Vector3 reflect(const Vector3& V, const Vector3& N) {
    return V - 2 * V.dot(N) * N;
}

inline Vector3 ray_colour(const Ray& r, const Scene& scene, const HittableList& world, int depth) {
    if (depth <= 0) {
        return Vector3(0, 0, 0);
    }
    HitRecord rec;
    if (!world.intersect(r, 0.001, 100000.0, rec)) {
        // background colour
        return Vector3(0.5, 0.7, 1.0);
    }


    Vector3 local_colour = blinn_phong_shade(rec, scene, world, r);

    Vector3 reflected_colour(0, 0, 0);
    if (rec.mat.reflectivity > 0) {
        Vector3 V_in = r.direction.normalize();
        Vector3 N = rec.normal.normalize();
        Vector3 reflect_dir = reflect(V_in, N).normalize();

        Ray reflect_ray(rec.point, reflect_dir);
        reflected_colour = ray_colour(reflect_ray, scene, world, depth - 1);
    }

    Vector3 refracted_colour(0, 0, 0);
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

            Ray refract_ray(rec.point, refract_dir.normalize());
            refracted_colour = ray_colour(refract_ray, scene, world, depth - 1);
        } else {
            Vector3 V = r.direction.normalize();
            Vector3 reflect_dir = reflect(V, rec.normal).normalize();
            Ray reflect_ray(rec.point, reflect_dir);
            refracted_colour = ray_colour(reflect_ray, scene, world, depth - 1);
        }
    }

    Vector3 final_colour = (local_colour * (1.0 - rec.mat.reflectivity - rec.mat.transparency))
                         + (reflected_colour * rec.mat.reflectivity)
                         + (refracted_colour * rec.mat.transparency);

    return final_colour;
}

inline Pixel final_colour_to_pixel(const Vector3& colour_vec) {
    auto clamp = [](double val) { return std::max(0.0, std::min(1.0, val)); };
    unsigned char r = static_cast<unsigned char>(clamp(colour_vec.x) * 255.0);
    unsigned char g = static_cast<unsigned char>(clamp(colour_vec.y) * 255.0);
    unsigned char b = static_cast<unsigned char>(clamp(colour_vec.z) * 255.0);
    return {r, g, b};
}

#endif //B216602_TRACER_H