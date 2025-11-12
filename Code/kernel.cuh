#pragma once

#include "device_types.cuh"
#include <cuda_runtime.h>
#include <cfloat>

#define MAX_RECURSION_DEPTH 10
#define EPSILON 0.0001
#define SMALL_FLOAT 1e-10


template<typename T>
__device__ inline void swap(T& a, T& b) {
    T temp = a;
    a = b;
    b = temp;
}

__device__ inline Vector3 reflect(const Vector3& V, const Vector3& N) {
    return V - 2 * V.dot(N) * N;
}

__device__ inline Vector3 component_wise_multiply(const Vector3& a, const Vector3& b) {
    return Vector3(a.x * b.x, a.y * b.y, a.z * b.z);
}


__device__ bool intersect_sphere(const Sphere& sphere, const Ray& ray, double t_min, double t_max, HitRecord& rec) {
    Ray object_ray(sphere.inverse_transform.transformPoint(ray.origin),
                   sphere.inverse_transform.transformVector(ray.direction));

    Vector3 oc = object_ray.origin;
    double a = object_ray.direction.dot(object_ray.direction);
    double half_b = oc.dot(object_ray.direction);
    double c = oc.dot(oc) - 1.0;
    double discriminant = half_b * half_b - a * c;

    if (discriminant < 0) {
        return false;
    }

    double sqrtd = sqrt(discriminant);
    double root = (-half_b - sqrtd) / a;

    if (root < t_min || root > t_max) {
        root = (-half_b + sqrtd) / a;
        if (root < t_min || root > t_max) {
            return false;
        }
    }

    rec.t = root;
    Vector3 obj_point = object_ray.point_at_parameter(rec.t);
    Vector3 obj_normal = obj_point;

    rec.point = sphere.transform.transformPoint(obj_point);
    rec.normal = sphere.inverse_transpose.transformVector(obj_normal).normalize();
    rec.mat = sphere.mat;

    if (ray.direction.dot(rec.normal) > 0.0) {
        rec.normal = -rec.normal;
    }

    return true;
}


__device__ bool intersect_cube(const Cube& cube, const Ray& ray, double t_min, double t_max, HitRecord& rec) {
    Ray object_ray(cube.inverse_transform.transformPoint(ray.origin),
                   cube.inverse_transform.transformVector(ray.direction));

    Vector3 inv_dir = Vector3(1.0 / object_ray.direction.x, 1.0 / object_ray.direction.y, 1.0 / object_ray.direction.z);
    Vector3 min_bounds(-1.0, -1.0, -1.0);
    Vector3 max_bounds(1.0, 1.0, 1.0);

    double tmin_x = (min_bounds.x - object_ray.origin.x) * inv_dir.x;
    double tmax_x = (max_bounds.x - object_ray.origin.x) * inv_dir.x;
    if (tmin_x > tmax_x) swap(tmin_x, tmax_x);

    double tmin_y = (min_bounds.y - object_ray.origin.y) * inv_dir.y;
    double tmax_y = (max_bounds.y - object_ray.origin.y) * inv_dir.y;
    if (tmin_y > tmax_y) swap(tmin_y, tmax_y);

    if ((tmin_x > tmax_y) || (tmin_y > tmax_x)) return false;

    double t_enter = fmax(tmin_x, tmin_y);
    double t_exit = fmin(tmax_x, tmax_y);

    double tmin_z = (min_bounds.z - object_ray.origin.z) * inv_dir.z;
    double tmax_z = (max_bounds.z - object_ray.origin.z) * inv_dir.z;
    if (tmin_z > tmax_z) swap(tmin_z, tmax_z);

    if ((t_enter > tmax_z) || (tmin_z > t_exit)) return false;

    t_enter = fmax(t_enter, tmin_z);
    t_exit = fmin(t_exit, tmax_z);

    double t_hit = t_enter;
    if (t_hit < t_min || t_hit > t_max) {
        t_hit = t_exit;
        if (t_hit < t_min || t_hit > t_max) return false;
    }

    rec.t = t_hit;
    Vector3 obj_point = object_ray.point_at_parameter(rec.t);

    Vector3 obj_normal;
    const double bias = 0.0001;
    if (abs(obj_point.x - 1.0) < bias) obj_normal = Vector3(1, 0, 0);
    else if (abs(obj_point.x + 1.0) < bias) obj_normal = Vector3(-1, 0, 0);
    else if (abs(obj_point.y - 1.0) < bias) obj_normal = Vector3(0, 1, 0);
    else if (abs(obj_point.y + 1.0) < bias) obj_normal = Vector3(0, -1, 0);
    else if (abs(obj_point.z - 1.0) < bias) obj_normal = Vector3(0, 0, 1);
    else obj_normal = Vector3(0, 0, -1);

    rec.point = cube.transform.transformPoint(obj_point);
    rec.normal = cube.inverse_transpose.transformVector(obj_normal).normalize();
    rec.mat = cube.mat;

    if (ray.direction.dot(rec.normal) > 0.0) {
        rec.normal = -rec.normal;
    }

    return true;
}

__device__ bool ray_triangle_intersect(
    const Ray& ray, double t_min, double t_max,
    const Vector3& v0, const Vector3& edge1, const Vector3& edge2, double& out_t)
{
    // E1 and E2 are edge1 (v1 - v0) and edge2 (v2 - v0)
    // Ray direction is R_d
    // The triangle plane is defined by (v0, v1, v2)

    Vector3 pvec = ray.direction.cross(edge2);
    double det = edge1.dot(pvec);

    // If det is near zero, ray is parallel to triangle plane.
    if (abs(det) < SMALL_FLOAT)
        return false;

    double invDet = 1.0 / det;
    Vector3 tvec = ray.origin - v0;
    double u = tvec.dot(pvec) * invDet;

    if (u < 0.0 || u > 1.0)
        return false;

    Vector3 qvec = tvec.cross(edge1);
    double v = ray.direction.dot(qvec) * invDet;

    if (v < 0.0 || u + v > 1.0)
        return false;

    // Calculate t
    out_t = edge2.dot(qvec) * invDet;

    // Check if t is within the valid range
    if (out_t > t_min && out_t < t_max) {
        return true;
    }

    return false;
}

__device__ bool intersect_plane(const Plane& plane, const Ray& ray, double t_min, double t_max, HitRecord& rec) {
    double t1 = DBL_MAX, t2 = DBL_MAX;
    bool hit1, hit2;

    // Triangle 1: p0, p1, p2
    Vector3 t1_v0 = plane.p0;
    Vector3 t1_edge1 = plane.p1 - plane.p0;
    Vector3 t1_edge2 = plane.p2 - plane.p0;

    // Triangle 2: p2, p3, p0
    Vector3 t2_v0 = plane.p1;
    Vector3 t2_edge1 = plane.p3 - plane.p1;
    Vector3 t2_edge2 = plane.p2 - plane.p1;

    // Check intersection with the first triangle
    hit1 = ray_triangle_intersect(ray, t_min, t_max, t1_v0, t1_edge1, t1_edge2, t1);

    // If hit, update t_max
    double current_t_max = hit1 ? t1 : t_max;

    // Check intersection with the second triangle
    hit2 = ray_triangle_intersect(ray, t_min, current_t_max, t2_v0, t2_edge1, t2_edge2, t2);

    if (!hit1 && !hit2) {
        return false;
    }

    // Determine the closest hit (if both hit)
    double t_hit;
    if (hit1 && hit2) {
        t_hit = fmin(t1, t2);
    } else if (hit1) {
        t_hit = t1;
    } else { // hit2 must be true
        t_hit = t2;
    }

    // Fill HitRecord
    rec.t = t_hit;
    rec.point = ray.point_at_parameter(t_hit);
    rec.normal = plane.normal; // Normal is consistent across the plane
    rec.mat = plane.mat;

    // Set the final normal orientation
    if (ray.direction.dot(rec.normal) > 0.0) {
        rec.normal = -rec.normal;
    }

    return true;
}


__device__ bool intersect_world(
    const Ray& ray,
    double t_min,
    double t_max,
    HitRecord& rec,
    const Sphere* d_spheres, int num_spheres,
    const Cube* d_cubes, int num_cubes,
    const Plane* d_planes, int num_planes
) {
    HitRecord temp_rec;
    bool hit_anything = false;
    double closest_so_far = t_max;

    // Iterate over spheres
    for (int i = 0; i < num_spheres; ++i) {
        if (intersect_sphere(d_spheres[i], ray, t_min, closest_so_far, temp_rec)) {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            rec = temp_rec;
        }
    }

    // Iterate over cubes
    for (int i = 0; i < num_cubes; ++i) {
        if (intersect_cube(d_cubes[i], ray, t_min, closest_so_far, temp_rec)) {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            rec = temp_rec;
        }
    }

    // Iterate over planes
    for (int i = 0; i < num_planes; ++i) {
        if (intersect_plane(d_planes[i], ray, t_min, closest_so_far, temp_rec)) {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            rec = temp_rec;
        }
    }
    return hit_anything;
}



__device__ Vector3 blinn_phong_shade(
    const HitRecord& rec,
    const Ray& view_ray,
    double exposure,
    bool enable_shadows,
    const PointLight* d_lights, int num_lights,
    const Sphere* d_spheres, int num_spheres,
    const Cube* d_cubes, int num_cubes,
    const Plane* d_planes, int num_planes
) {
    const Vector3 P = rec.point;
    const Vector3 N = rec.normal;
    const Vector3 V = (view_ray.origin - P).normalize();
    const Material& mat = rec.mat;

    // Ambient Component
    Vector3 global_ambient_light(0.2, 0.2, 0.2);
    Vector3 final_color_vec = component_wise_multiply(mat.ambient, global_ambient_light);

    // Loop over all lights
    for (int i = 0; i < num_lights; ++i) {
        const PointLight& light = d_lights[i];

        double shadow_factor = 1.0;
        if (enable_shadows) {
            Vector3 shadow_ray_dir = (light.position - P).normalize();
            double dist_to_light = (light.position - P).length();
            // Offset origin slightly to avoid self-intersection
            Ray shadow_ray(P + N * EPSILON, shadow_ray_dir);
            HitRecord shadow_rec;

            if (intersect_world(shadow_ray, 0.0, dist_to_light - EPSILON, shadow_rec,
                                d_spheres, num_spheres, d_cubes, num_cubes, d_planes, num_planes)) {
                shadow_factor = 0.0; // In shadow
            }
        }

        if (shadow_factor > 0.0) {
            Vector3 L = (light.position - P).normalize();
            Vector3 H = (L + V).normalize();
            Vector3 exposed_light_intensity = light.intensity * exposure;

            // Diffuse
            double L_dot_N = fmax(0.0, L.dot(N));
            Vector3 diffuse = component_wise_multiply(mat.diffuse, exposed_light_intensity) * L_dot_N;

            // Specular
            double H_dot_N = fmax(0.0, H.dot(N));
            Vector3 specular = component_wise_multiply(mat.specular, exposed_light_intensity) * pow(H_dot_N, mat.shininess);

            final_color_vec = final_color_vec + (diffuse + specular) * shadow_factor;
        }
    }
    return final_color_vec;
}


__device__ Vector3 ray_colour(
    Ray ray,
    double exposure,
    bool enable_shadows,
    const PointLight* d_lights, int num_lights,
    const Sphere* d_spheres, int num_spheres,
    const Cube* d_cubes, int num_cubes,
    const Plane* d_planes, int num_planes
) {
    Vector3 final_colour(0, 0, 0);
    Vector3 ray_attenuation(1, 1, 1);


    for (int depth = 0; depth < MAX_RECURSION_DEPTH; ++depth) {
        HitRecord rec;

        if (!intersect_world(ray, EPSILON, DBL_MAX, rec, d_spheres, num_spheres, d_cubes, num_cubes, d_planes, num_planes)) {
            final_colour = final_colour + component_wise_multiply(ray_attenuation, Vector3(0.5, 0.7, 1.0)); // Background
            break;
        }

        Vector3 local_colour = blinn_phong_shade(rec, ray, exposure, enable_shadows,
                                                 d_lights, num_lights,
                                                 d_spheres, num_spheres, d_cubes, num_cubes, d_planes, num_planes);


        bool has_reflection = rec.mat.reflectivity > 0;

        final_colour = final_colour + component_wise_multiply(ray_attenuation, local_colour) * (1.0 - rec.mat.reflectivity);

        if (has_reflection) {
            ray_attenuation = ray_attenuation * rec.mat.reflectivity;

            Vector3 V_in = ray.direction.normalize();
            Vector3 N = rec.normal.normalize();
            ray = Ray(rec.point + N * EPSILON, reflect(V_in, N));
        } else {
            break;
        }

    }

    return final_colour;
}


__global__ void render_kernel(
    unsigned char* d_image_data,
    int width, int height,
    CUDACamera camera,
    double exposure,
    bool enable_shadows,
    const PointLight* d_lights, int num_lights,
    const Sphere* d_spheres, int num_spheres,
    const Cube* d_cubes, int num_cubes,
    const Plane* d_planes, int num_planes
) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= width || y >= height) {
        return;
    }

    float u = (static_cast<float>(width - 1 - x) + 0.5f) / width;
    float v = (static_cast<float>(height - 1 - y) + 0.5f) / height;

    Ray ray = camera.get_ray(u, v);

    Vector3 final_color_vec = ray_colour(ray, exposure, enable_shadows,
                                         d_lights, num_lights,
                                         d_spheres, num_spheres,
                                         d_cubes, num_cubes,
                                         d_planes, num_planes);

    auto clamp = [](double val) { return fmax(0.0, fmin(1.0, val)); };
    unsigned char r = static_cast<unsigned char>(clamp(final_color_vec.x) * 255.999);
    unsigned char g = static_cast<unsigned char>(clamp(final_color_vec.y) * 255.999);
    unsigned char b = static_cast<unsigned char>(clamp(final_color_vec.z) * 255.999);


    int flipped_y = height - 1 - y;
    int pixel_index = (flipped_y * width + x) * 3; // *3 for R,G,B
    d_image_data[pixel_index + 0] = r;
    d_image_data[pixel_index + 1] = g;
    d_image_data[pixel_index + 2] = b;
}