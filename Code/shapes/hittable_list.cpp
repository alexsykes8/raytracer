//
// Created by alex on 29/10/2025.
//

#include "../shapes/hittable_list.h"

bool HittableList::intersect(const Ray &ray, double t_min, double t_max, HitRecord& rec) const {
    HitRecord temp_rec;
    bool hit_anything = false;
    double closest_so_far = t_max;

    for (const auto& object : objects) {
        if (object -> intersect(ray, t_min, closest_so_far, temp_rec)) {
            hit_anything = true;
            closest_so_far = temp_rec.t;
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