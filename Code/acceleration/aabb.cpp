#include "../acceleration/aabb.h"

// AABB intersection using the slab method
bool AABB::intersect(const Ray& ray, double tmin, double tmax) const {
    // Inverse direction components
    Vector3 inv_direction(1.0 / ray.direction.x, 1.0 / ray.direction.y, 1.0 / ray.direction.z);

    // Calculate t-intervals for each slab
    double tx1 = (min_point.x - ray.origin.x) * inv_direction.x;
    double tx2 = (max_point.x - ray.origin.x) * inv_direction.x;

    tmin = std::max(tmin, std::min(tx1, tx2));
    tmax = std::min(tmax, std::max(tx1, tx2));

    double ty1 = (min_point.y - ray.origin.y) * inv_direction.y;
    double ty2 = (max_point.y - ray.origin.y) * inv_direction.y;

    tmin = std::max(tmin, std::min(ty1, ty2));
    tmax = std::min(tmax, std::max(ty1, ty2));

    double tz1 = (min_point.z - ray.origin.z) * inv_direction.z;
    double tz2 = (max_point.z - ray.origin.z) * inv_direction.z;

    tmin = std::max(tmin, std::min(tz1, tz2));
    tmax = std::min(tmax, std::max(tz1, tz2));

    // If tmin >= tmax, the ray missed or only hit edges/corners at the boundary
    return tmin < tmax;
}

// Combine two bounding boxes into one larger box
AABB AABB::combine(const AABB& box1, const AABB& box2) {
    Vector3 small(
        std::min(box1.min_point.x, box2.min_point.x),
        std::min(box1.min_point.y, box2.min_point.y),
        std::min(box1.min_point.z, box2.min_point.z)
    );
    Vector3 big(
        std::max(box1.max_point.x, box2.max_point.x),
        std::max(box1.max_point.y, box2.max_point.y),
        std::max(box1.max_point.z, box2.max_point.z)
    );
    return AABB(small, big);
}