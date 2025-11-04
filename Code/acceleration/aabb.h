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

    // AABB uses the slab test method for intersection.
    /*
     * @param ray The ray to test.
     * @param tmin Minimum valid t value for the ray.
     * @param tmax Maximum valid t value for the ray.
     * @return true if the ray hits the box within the [tmin, tmax] range, false otherwise.
     */
    bool intersect(const Ray& ray, double tmin, double tmax) const;

    /*
     * @brief Creates a new AABB that encompasses two existing AABBs.
     * @param box1 The first bounding box.
     * @param box2 The second bounding box.
     * @return A new AABB containing both box1 and box2.
     */
    static AABB combine(const AABB& box1, const AABB& box2);
};

#endif //B216602_AABB_H