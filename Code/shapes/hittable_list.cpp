//
// Created by alex on 29/10/2025.
//

#include "../shapes/hittable_list.h"

// t_min is the minimum valid distance for a hit. It makes the ray start just about the surface of the object it originated from to prevent self-intersection
// t_max is the maximum valid distance for a hit, the horizon of the scene.
bool HittableList::intersect(const Ray &ray, double t_min, double t_max, HitRecord& rec) const {
    HitRecord temp_rec;
    bool hit_anything = false;
    double closest_so_far = t_max;

    for (const auto& object : objects) {
        // performs a polymorphic intersection test against a single object in the scene. It uses whichever intersect method is relevant to the object type (ie. cube sphere plane)
        if (object -> intersect(ray, t_min, closest_so_far, temp_rec)) {
            hit_anything = true;
            // reduces t_max according to the distance to the closest object found (t is distance from ray origin to intersection)
            closest_so_far = temp_rec.t;
            // preserves the data for the closest object encountered to the camera
            rec = temp_rec;

        }
    }

    return hit_anything;
}

bool HittableList::getBoundingBox(AABB &output_box) const {
    if (objects.empty()) {
        return false;
    }
    AABB temp_box;
    bool first_box = true;

    for (const auto& object : objects) {
        if (!object->getBoundingBox(temp_box)) {
            return false;
        }
        output_box = first_box ? temp_box : AABB::combine(output_box, temp_box);
        first_box = false;
    }

    return true;
}

bool HittableList::any_hit(const Ray &ray, double t_min, double t_max) const {
    for (const auto& object : objects) {
        if (object->any_hit(ray, t_min, t_max)) {
            return true; // Found a blocker, stop checking other objects
        }
    }
    return false;
}