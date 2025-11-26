#include "../acceleration/aabb.h"

// checks for an intersection between a ray and the bounding box using the slab test method.
bool AABB::intersect(const Ray& ray, double tmin, double tmax) const {
    // iterates through the x, y, and z axes.
    for (int a = 0; a < 3; a++) {
        // pre-calculates the inverse of the ray's direction component for the current axis to avoid multiple divisions.
        double invD = 1.0 / (a == 0 ? ray.direction.x : (a == 1 ? ray.direction.y : ray.direction.z));
        // gets the ray's origin component for the current axis.
        double origin = (a == 0 ? ray.origin.x : (a == 1 ? ray.origin.y : ray.origin.z));
        // gets the box's min and max components for the current axis.
        double min_p = (a == 0 ? min_point.x : (a == 1 ? min_point.y : min_point.z));
        double max_p = (a == 0 ? max_point.x : (a == 1 ? max_point.y : max_point.z));

        // calculates the intersection distances with the two slab planes for the current axis.
        // equation: t0 = (min_p - origin) * invD
        double t0 = (min_p - origin) * invD;
        // equation: t1 = (max_p - origin) * invD
        double t1 = (max_p - origin) * invD;

        // ensures t0 is the smaller (entry) distance and t1 is the larger (exit) distance.
        if (invD < 0.0) {
            std::swap(t0, t1);
        }

        // updates the overall intersection range by taking the maximum of the near distances.
        tmin = t0 > tmin ? t0 : tmin;
        // updates the overall intersection range by taking the minimum of the far distances.
        tmax = t1 < tmax ? t1 : tmax;

        // if the intersection range becomes invalid (max < min), the ray has missed the box.
        if (tmax <= tmin) {
            return false;
        }
    }
    // if the loop completes, the ray intersects the box within the valid range.
    return true;
}

// combines two bounding boxes into a single one that encloses both.
AABB AABB::combine(const AABB& box1, const AABB& box2) {
    // calculates the component-wise minimum of the two boxes' min points.
    Vector3 small(
        std::min(box1.min_point.x, box2.min_point.x),
        std::min(box1.min_point.y, box2.min_point.y),
        std::min(box1.min_point.z, box2.min_point.z)
    );
    // calculates the component-wise maximum of the two boxes' max points.
    Vector3 big(
        std::max(box1.max_point.x, box2.max_point.x),
        std::max(box1.max_point.y, box2.max_point.y),
        std::max(box1.max_point.z, box2.max_point.z)
    );
    // returns a new aabb created from the new min and max points.
    return AABB(small, big);
}
