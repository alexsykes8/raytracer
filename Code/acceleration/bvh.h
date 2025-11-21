#ifndef B216602_BVH_H
#define B216602_BVH_H

#include "../shapes/hittable.h"
#include "../shapes/hittable_list.h"
#include "aabb.h"
#include <vector>
#include <memory>
#include <algorithm>

class BVHNode : public Shape {
public:

    // Two constructors, one takes a range from hittable objects and one takes the whole list of hittable objects.

    // Builds the BVH tree from a list of hittable objects.
    BVHNode(std::vector<std::shared_ptr<Shape>>& objects, size_t start, size_t end);

    // Alternative constructor that takes a HittableList directly
    BVHNode(HittableList& list) : BVHNode(list.objects, 0, list.objects.size()) {}


    // Checks for intersection by testing the bounding box first, then children.
    virtual bool intersect(
        const Ray& ray,
        double t_min,
        double t_max,
        HitRecord& rec
    ) const override;


    // Returns the bounding box of this node.
    virtual bool getBoundingBox(AABB& output_box) const override;


private:
    std::shared_ptr<Shape> m_left;  // Left child node
    std::shared_ptr<Shape> m_right; // Right child node
    AABB m_box;                     // Bounding box containing left and right children
};

#endif //B216602_BVH_H