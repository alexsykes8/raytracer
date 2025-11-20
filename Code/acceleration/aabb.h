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
};

#endif //B216602_AABB_H