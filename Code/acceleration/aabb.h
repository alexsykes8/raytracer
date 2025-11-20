#ifndef B216602_AABB_H
#define B216602_AABB_H

#include "../vector3.h"
#include "../ray.h"
#include <algorithm> // For std::min/max

class AABB {
public:
    Vector3 min_point;
    Vector3 max_point;

    // creates an empty/invalid box
    AABB() {}

    // Constructor with min and max points
    AABB(const Vector3& min_p, const Vector3& max_p) : min_point(min_p), max_point(max_p) {}

    bool intersect(const Ray& ray, double tmin, double tmax) const;


    static AABB combine(const AABB& box1, const AABB& box2);

    static void updateBounds(const Vector3& p, Vector3& min_p, Vector3& max_p) {
        min_p.x = std::min(min_p.x, p.x);
        min_p.y = std::min(min_p.y, p.y);
        min_p.z = std::min(min_p.z, p.z);
        max_p.x = std::max(max_p.x, p.x);
        max_p.y = std::max(max_p.y, p.y);
        max_p.z = std::max(max_p.z, p.z);
    }
};

#endif //B216602_AABB_H