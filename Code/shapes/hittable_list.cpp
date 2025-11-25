//
// Created by alex on 29/10/2025.
//

#include "../shapes/hittable_list.h"

// checks for the closest intersection of a ray with any object in the list within a given distance range.
bool HittableList::intersect(const Ray &ray, double t_min, double t_max, HitRecord& rec) const {
    // a temporary record to store intersection data for each object.
    HitRecord temp_rec;
    // a flag to track if any object has been hit.
    bool hit_anything = false;
    // initialises the closest hit distance to the maximum allowed distance.
    double closest_so_far = t_max;

    // 'auto' means that the compiler deduces the type of a variable from its initialiser.
    // iterates through each object in the list.
    for (const auto& object : objects) {
        // performs a polymorphic intersection test against a single object.
        // the '->' operator is used to access members of an object through a pointer.
        // the intersection range is progressively narrowed to [t_min, closest_so_far] to find the nearest hit.
        if (object -> intersect(ray, t_min, closest_so_far, temp_rec)) {
            // if an intersection is found, update the state.
            hit_anything = true;
            // updates the closest hit distance to the new, nearer intersection point.
            closest_so_far = temp_rec.t;
            // copies the hit data of the closest object found so far into the final record.
            rec = temp_rec;
        }
    }

    return hit_anything;
}

bool HittableList::getBoundingBox(AABB &output_box) const {
    // if there are no objects, a bounding box cannot be created.
    if (objects.empty()) {
        return false;
    }
    // a temporary bounding box for individual objects.
    AABB temp_box;
    // a flag to handle the first object's bounding box.
    bool first_box = true;

    for (const auto& object : objects) {
        // gets the bounding box for the current object.
        if (!object->getBoundingBox(temp_box)) {
            return false;
        }
        // combines the current object's box with the overall list's box.
        output_box = first_box ? temp_box : AABB::combine(output_box, temp_box);
        first_box = false;
    }

    return true;
}
